#include "psi.h"

#include <cstdint>
#include <random>
#include <thread>
#include <vector>
#include "bfv.h"
#include "crt.h"
#include "kuckoo.h"
#include "packing.h"
#include "party.h"
#include "random.h"
#include "seal/seal.h"

using namespace cuckoo;
using namespace fhe;
using namespace math;
using namespace seal;
using namespace std;

namespace psi
{

void computeIntersection // single-thread
(
    vector<vector<Ciphertext>> & results, // return masked intersection under Sender's key
    vector<vector<Ciphertext>> & randoms,  // return random masks under Receiver's key
    const Party & receiver,
    const Kuckoo & cuckoo,
    const vector<Ciphertext> & encrypted_table,
    const CrtParams & crt,
    uint64_t sender_eta,
    const SEALContext * sender_context_ptr,
    const BatchEncoder * sender_encoder_ptr,
    const Evaluator * sender_evaluator_ptr,
    const RelinKeys * sender_relinkeys_ptr,
    const BatchEncoder * receiver_encoder_ptr,
    const Encryptor * receiver_encryptor_ptr,
    uint64_t receiver_dummy
)
{
    const auto & receiver_set = receiver.getSet();
    const uint64_t num_hashes = cuckoo.getNumHashes();
    const uint64_t k = crt.mi.size();
    const uint64_t sender_n = sender_encoder_ptr->slot_count();
    const uint64_t receiver_n = receiver_encoder_ptr->slot_count();
    const uint64_t return_width = sender_eta + 1;

    results.resize(receiver_set.size(), vector<Ciphertext>(return_width));
    randoms.resize(receiver_set.size(), vector<Ciphertext>(return_width));

    // for each entry in Receiver's set
    for (uint64_t i=0; i<receiver_set.size(); i++)
    {
        const auto & entry = receiver_set[i];
        auto [y_r, ct_pslot, indices] = cuckoo.getIndices(entry);

        // create subtraction matrix
        vector<vector<Ciphertext>> subtractions(return_width);
        {   
            // resize subtractions to make multiply_many easy with partitioning parameter
            uint64_t subtraction_size = num_hashes / return_width;
            uint64_t subtraction_remainder = num_hashes % return_width;
            for (uint64_t j=0; j<return_width; j++) subtractions[j].resize(subtraction_size + bool(j < subtraction_remainder));
        }

        // for each hash function, subtract the corresponding slot
        for (uint64_t j=0; j<num_hashes; j++)
        {
            auto & index = indices[j];
            // Create plaintext polynomial for subtraction
            uint64_t ct_index = index / sender_n;
            uint64_t ct_bslot = index % sender_n;
            auto & ct = encrypted_table[ct_index];
            auto slot = ct_bslot * k + ct_pslot;
            vector<uint64_t> v(k*sender_n, receiver_dummy);
            v[slot] = y_r;
            Plaintext pt;
            packEncode(pt, v, crt, sender_encoder_ptr);

            // Homomorphically compute the difference
            sender_evaluator_ptr->sub_plain(ct, pt, subtractions[j % return_width][j / return_width]);
        }

        for (uint64_t j=0; j<return_width; j++)
        {
            // Depth-optimized homomorphic multiplications respecting the partitioning parameter
            sender_evaluator_ptr->multiply_many(subtractions[j], *sender_relinkeys_ptr, results[i][j]);

            // Add random values to the result
            auto random_values = randomVector(receiver_n, 0, crt.M);
            Plaintext sender_random_pt;
            sender_encoder_ptr->encode(random_values, sender_random_pt);
            sender_evaluator_ptr->add_plain_inplace(results[i][j], sender_random_pt);

            // Modulus switch
            sender_evaluator_ptr->mod_switch_to_inplace(results[i][j], sender_context_ptr->last_parms_id());

            // Encrypt random values with Receiver's key
            Plaintext receiver_random_pt;
            receiver_encoder_ptr->encode(random_values, receiver_random_pt);
            receiver_encryptor_ptr->encrypt_symmetric(receiver_random_pt, randoms[i][j]);
        }
    }
}

void computeIntersection // multi-thread
(
    vector<vector<Ciphertext>> & results, // return masked intersection under Sender's key
    vector<vector<Ciphertext>> & randoms,  // return random masks under Receiver's key
    const Party & receiver,
    const Kuckoo & cuckoo,
    const vector<Ciphertext> & encrypted_table,
    const CrtParams & crt,
    uint64_t sender_eta,
    const SEALContext * sender_context_ptr,
    const BatchEncoder * sender_encoder_ptr,
    const Evaluator * sender_evaluator_ptr,
    const RelinKeys * sender_relinkeys_ptr,
    const BatchEncoder * receiver_encoder_ptr,
    const Encryptor * receiver_encryptor_ptr,
    uint64_t receiver_dummy,
    uint64_t num_threads
)
{
    const auto & receiver_set = receiver.getSet();
    const uint64_t return_width = sender_eta + 1;

    results.resize(receiver_set.size(), vector<Ciphertext>(return_width));
    randoms.resize(receiver_set.size(), vector<Ciphertext>(return_width));

    uint64_t outer_threads = min(num_threads, receiver_set.size());
    uint64_t inner_threads = num_threads / outer_threads + bool(num_threads % outer_threads);

    vector<thread> threads(outer_threads);
    for (uint64_t t=0; t<outer_threads; t++)
    {
        threads[t] = thread
        ([
            t, outer_threads, inner_threads, &results, &randoms, &receiver_set, &cuckoo, &encrypted_table, &crt, return_width,
            sender_context_ptr, sender_encoder_ptr, sender_evaluator_ptr, sender_relinkeys_ptr, receiver_encoder_ptr, receiver_encryptor_ptr, receiver_dummy
        ]()
        {
            const uint64_t num_hashes = cuckoo.getNumHashes();

             // for each entry in Receiver's set
            for (uint64_t i=t; i<receiver_set.size(); i+=outer_threads)
            {
                const auto & entry = receiver_set[i];
                auto [y_r, ct_pslot, indices] = cuckoo.getIndices(entry);

                // create subtraction matrix
                vector<vector<Ciphertext>> subtractions(return_width);
                {   
                    // resize subtractions to make multiply_many easy with partitioning parameter
                    uint64_t subtraction_size = num_hashes / return_width;
                    uint64_t subtraction_remainder = num_hashes % return_width;
                    for (uint64_t j=0; j<return_width; j++) subtractions[j].resize(subtraction_size + bool(j < subtraction_remainder));
                }

                {
                    // Homomorphic subtraction
                    uint64_t internal_threads = min(inner_threads, num_hashes);
                    vector<thread> threads(internal_threads);
                    for (uint64_t u=0; u<internal_threads; u++)
                    {
                        threads[u] = thread
                        ([
                            u, internal_threads, &subtractions, &y_r, &ct_pslot, &indices, &encrypted_table,
                            &crt, &sender_encoder_ptr, &sender_evaluator_ptr, receiver_dummy
                        ]()
                        {
                            const uint64_t num_hashes = indices.size();
                            const uint64_t k = crt.mi.size();
                            const uint64_t sender_n = sender_encoder_ptr->slot_count();
                            const uint64_t return_width = subtractions.size();

                            // for each hash function, subtract the corresponding slot
                            for (uint64_t j=u; j<num_hashes; j+=internal_threads)
                            {
                                // Create plaintext polynomial for subtraction
                                auto & index = indices[j];
                                uint64_t ct_index = index / sender_n;
                                uint64_t ct_bslot = index % sender_n;
                                auto & ct = encrypted_table[ct_index];
                                auto slot = ct_bslot * k + ct_pslot;
                                vector<uint64_t> v(k*sender_n, receiver_dummy);
                                v[slot] = y_r;
                                Plaintext pt;
                                packEncode(pt, v, crt, sender_encoder_ptr);

                                // Homomorphically compute the difference
                                sender_evaluator_ptr->sub_plain(ct, pt, subtractions[j % return_width][j / return_width]);
                            }
                        });
                    }
                    for (auto & thread : threads) thread.join();
                }

                {
                    // Homomorphic multiplication and randomness addition
                    uint64_t internal_threads = min(inner_threads, return_width);
                    vector<thread> threads(internal_threads);
                    for (uint64_t u=0; u<internal_threads; u++)
                    {
                        threads[u] = thread(
                        [
                            u, internal_threads, &results, &randoms, i, &subtractions, &crt,
                            &sender_context_ptr, &sender_encoder_ptr, &sender_evaluator_ptr, &sender_relinkeys_ptr,
                            &receiver_encoder_ptr, &receiver_encryptor_ptr
                        ]()
                        {
                            const uint64_t receiver_n = receiver_encoder_ptr->slot_count();
                            const uint64_t return_width = subtractions.size();

                            for (uint64_t j=u; j<return_width; j+=internal_threads)
                            {
                                // Depth-optimized homomorphic multiplications respecting the partitioning parameter
                                sender_evaluator_ptr->multiply_many(subtractions[j], *sender_relinkeys_ptr, results[i][j]);

                                // Add random values to the result
                                auto random_values = randomVector(receiver_n, 0, crt.M);
                                Plaintext sender_random_pt;
                                sender_encoder_ptr->encode(random_values, sender_random_pt);
                                sender_evaluator_ptr->add_plain_inplace(results[i][j], sender_random_pt);

                                // Modulus switch
                                sender_evaluator_ptr->mod_switch_to_inplace(results[i][j], sender_context_ptr->last_parms_id());

                                // Encrypt random values with Receiver's key
                                Plaintext receiver_random_pt;
                                receiver_encoder_ptr->encode(random_values, receiver_random_pt);
                                receiver_encryptor_ptr->encrypt_symmetric(receiver_random_pt, randoms[i][j]);
                            }
                        });
                    }
                    for (auto & thread : threads) thread.join();
                }
            }
        });
    }
    for (auto & thread : threads) thread.join();
}

vector<uint64_t> decryptIntersection // single-thread
(
    const vector<vector<Ciphertext>> & finals,
    const Party & receiver,
    const CrtParams & crt,
    const BatchEncoder * receiver_encoder_ptr,
    Decryptor * receiver_decryptor_ptr
)
{
    vector<bool> flag(receiver.getSet().size(), false);
    for (uint64_t i=0; i<finals.size(); i++)
    {
        for (uint64_t j=0; j<finals[i].size(); j++)
        {
            auto final_values = packDecrypt(finals[i][j], crt, receiver_encoder_ptr, receiver_decryptor_ptr);
            for (auto & value : final_values)
                if (!value) { flag[i] = true; break; }
            if (flag[i]) break;
        }
    }

    vector<uint64_t> intersection;
    for (uint64_t i=0; i<flag.size(); i++)
        if (flag[i]) intersection.push_back(receiver.getSet()[i]);
    return intersection;
}

vector<uint64_t> decryptIntersection // multi-thread
(
    const vector<vector<Ciphertext>> & finals,
    const Party & receiver,
    const CrtParams & crt,
    const BatchEncoder * receiver_encoder_ptr,
    Decryptor * receiver_decryptor_ptr,
    uint64_t num_threads
)
{
    vector<bool> flag(receiver.getSet().size(), false);

    uint64_t outer_threads = min(num_threads, finals.size());
    uint64_t inner_threads = num_threads / outer_threads + bool(num_threads % outer_threads);

    vector<thread> threads(outer_threads);
    for (uint64_t t=0; t<outer_threads; t++)
    {
        threads[t] = thread(
        [
            t, outer_threads, inner_threads, &flag, &finals, &receiver, &crt, &receiver_encoder_ptr, &receiver_decryptor_ptr
        ]()
        {
            for (uint64_t i=t; i<finals.size(); i+=outer_threads)
            {
                uint64_t internal_threads = min(inner_threads, finals[i].size());

                vector<thread> threads(internal_threads);
                for (uint64_t u=0; u<internal_threads; u++)
                {
                    threads[u] = thread(
                    [
                        u, internal_threads, i, &flag, &finals, &receiver, &crt, &receiver_encoder_ptr, &receiver_decryptor_ptr
                    ]()
                    {
                        for (uint64_t j=u; j<finals[i].size(); j+=internal_threads)
                        {
                            auto final_values = packDecrypt(finals[i][j], crt, receiver_encoder_ptr, receiver_decryptor_ptr);
                            for (auto & value : final_values)
                                if (!value) { flag[i] = true; break; }
                            if (flag[i]) break;
                        }
                    });
                }
                for (auto & thread : threads) thread.join();
            }
        });
    }
    for (auto & thread : threads) thread.join();

    vector<uint64_t> intersection;
    for (uint64_t i=0; i<flag.size(); i++)
        if (flag[i]) intersection.push_back(receiver.getSet()[i]);
    return intersection;
}

void recrypt // single-thread
( 
    vector<vector<Ciphertext>> & finals,
    const vector<vector<Ciphertext>> & results,
    const vector<vector<Ciphertext>> & randoms,
    const CrtParams & crt,
    uint64_t receiver_eta,
    const BatchEncoder * sender_encoder_ptr,
    Decryptor * sender_decryptor_ptr,
    const SEALContext * receiver_context_ptr,
    const BatchEncoder * receiver_encoder_ptr,
    const Evaluator * receiver_evaluator_ptr,
    const RelinKeys * receiver_relinkeys_ptr,
    const GaloisKeys * receiver_galoiskeys_ptr
)
{
    const uint64_t return_width = results[0].size();
    const uint64_t final_width = receiver_eta + 1;
    const uint64_t receiver_n = receiver_encoder_ptr->slot_count();

    finals.resize(results.size(), vector<Ciphertext>(final_width));

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint64_t> dist(0, receiver_n-1);

    for (uint64_t i=0; i<results.size(); i++)
    {
        vector<vector<Ciphertext>> subtractions(final_width);
        {
            // resize subtractions to make multiply_many easy with partitioning parameter
            uint64_t subtraction_size = return_width / final_width; 
            uint64_t subtraction_remainder = return_width % final_width;
            for (uint64_t j=0; j<final_width; j++) subtractions[j].resize(subtraction_size + bool(j < subtraction_remainder));
        }

        // Decrypt the result and subtract it from the random mask
        for (uint64_t j=0; j<return_width; j++)
        {
            // Decrypt the result and encode it under Receiver's key
            Plaintext sender_result_pt, receiver_result_pt;
            vector<uint64_t> sender_result;
            sender_decryptor_ptr->decrypt(results[i][j], sender_result_pt);
            sender_encoder_ptr->decode(sender_result_pt, sender_result);
            receiver_encoder_ptr->encode(sender_result, receiver_result_pt);

            // Subtract the random mask
            receiver_evaluator_ptr->sub_plain(randoms[i][j], receiver_result_pt, subtractions[j % final_width][j / final_width]);
        }

        for (uint64_t j=0; j<final_width; j++)
        {
            // Depth-optimized homomorphic multiplication respecting the partitioning parameter
            receiver_evaluator_ptr->multiply_many(subtractions[j], *receiver_relinkeys_ptr, finals[i][j]);
            
            // Multiply non-zero random values to the result
            auto random_values = randomVector(receiver_n, 1, crt.M, crt.mi);
            Plaintext receiver_random_pt;
            receiver_encoder_ptr->encode(random_values, receiver_random_pt);
            receiver_evaluator_ptr->multiply_plain_inplace(finals[i][j], receiver_random_pt);

            // Rotate the result
            uint64_t steps = dist(gen);
            rotate(finals[i][j], steps, receiver_n, receiver_evaluator_ptr, receiver_galoiskeys_ptr);

            // Modulus switch
            receiver_evaluator_ptr->mod_switch_to_inplace(finals[i][j], receiver_context_ptr->last_parms_id());
        }
    }
}

void recrypt // multi-thread
(
    vector<vector<Ciphertext>> & finals,
    const vector<vector<Ciphertext>> & results,
    const vector<vector<Ciphertext>> & randoms,
    const CrtParams & crt,
    uint64_t receiver_eta,
    const BatchEncoder * sender_encoder_ptr,
    Decryptor * sender_decryptor_ptr,
    const SEALContext * receiver_context_ptr,
    const BatchEncoder * receiver_encoder_ptr,
    const Evaluator * receiver_evaluator_ptr,
    const RelinKeys * receiver_relinkeys_ptr,
    const GaloisKeys * receiver_galoiskeys_ptr,
    uint64_t num_threads
)
{
    const uint64_t final_width = receiver_eta + 1;

    finals.resize(results.size(), vector<Ciphertext>(final_width));

    uint64_t outer_threads = min(num_threads, results.size());
    uint64_t inner_threads = num_threads / outer_threads + bool(num_threads % outer_threads);

    vector<thread> threads(outer_threads);
    for (uint64_t t=0; t<outer_threads; t++)
    {
        threads[t] = thread(
        [
            t, outer_threads, inner_threads, &finals, &results, &randoms, &crt, final_width, sender_encoder_ptr, sender_decryptor_ptr,
            receiver_context_ptr, receiver_encoder_ptr, receiver_evaluator_ptr, receiver_relinkeys_ptr, receiver_galoiskeys_ptr
        ]()
        {
            const uint64_t return_width = results[t].size();

            for (uint64_t i=t; i<results.size(); i+=outer_threads)
            {
                vector<vector<Ciphertext>> subtractions(final_width);
                {
                    // resize subtractions to make multiply_many easy with partitioning parameter
                    uint64_t subtraction_size = return_width / final_width; 
                    uint64_t subtraction_remainder = return_width % final_width;
                    for (uint64_t j=0; j<final_width; j++) subtractions[j].resize(subtraction_size + bool(j < subtraction_remainder));
                }

                // Homomorphic subtraction
                {
                    uint64_t internal_threads = min(inner_threads, return_width);
                    vector<thread> threads(internal_threads);
                    for (uint64_t u=0; u<internal_threads; u++)
                    {
                        threads[u] = thread(
                        [
                            u, internal_threads, &subtractions, &results, &randoms, i, final_width, return_width,
                            &sender_decryptor_ptr, &sender_encoder_ptr, &receiver_encoder_ptr, &receiver_evaluator_ptr
                        ]()
                        {
                            for (uint64_t j=u; j<return_width; j+=internal_threads)
                            {
                                // Decrypt the result and subtract it from the random mask
                                Plaintext sender_result_pt, receiver_result_pt;
                                vector<uint64_t> sender_result;
                                sender_decryptor_ptr->decrypt(results[i][j], sender_result_pt);
                                sender_encoder_ptr->decode(sender_result_pt, sender_result);
                                receiver_encoder_ptr->encode(sender_result, receiver_result_pt);

                                // Subtract the random mask
                                receiver_evaluator_ptr->sub_plain(randoms[i][j], receiver_result_pt, subtractions[j % final_width][j / final_width]);
                            }
                        });
                    }
                    for (auto & thread : threads) thread.join();
                }
                
                {
                    uint64_t internal_threads = min(inner_threads, final_width);
                    vector<thread> threads(internal_threads);
                    for (uint64_t u=0; u<internal_threads; u++)
                    {
                        threads[u] = thread(
                        [
                            u, internal_threads, &finals, &subtractions, i, &crt, final_width, &receiver_context_ptr,
                            &receiver_encoder_ptr, &receiver_evaluator_ptr, &receiver_relinkeys_ptr, &receiver_galoiskeys_ptr
                        ]()
                        {
                            const uint64_t receiver_n = receiver_encoder_ptr->slot_count();

                            random_device rd;
                            mt19937 gen(rd());
                            uniform_int_distribution<uint64_t> dist(0, receiver_n-1);

                            for (uint64_t j=u; j<final_width; j+=internal_threads)
                            {
                                // Depth-optimized homomorphic multiplication respecting the partitioning parameter
                                receiver_evaluator_ptr->multiply_many(subtractions[j], *receiver_relinkeys_ptr, finals[i][j]);

                                // Multiply non-zero random values to the result
                                auto random_values = randomVector(receiver_n, 1, crt.M, crt.mi);
                                Plaintext receiver_random_pt;
                                receiver_encoder_ptr->encode(random_values, receiver_random_pt);
                                receiver_evaluator_ptr->multiply_plain_inplace(finals[i][j], receiver_random_pt);

                                // Rotate the result
                                uint64_t steps = dist(gen);
                                rotate(finals[i][j], steps, receiver_n, receiver_evaluator_ptr, receiver_galoiskeys_ptr);

                                // Modulus switch
                                receiver_evaluator_ptr->mod_switch_to_inplace(finals[i][j], receiver_context_ptr->last_parms_id());
                            }
                        });
                        for (auto & thread : threads) thread.join();
                    }
                }
            }
        });
    }
    for (auto & thread : threads) thread.join();
}

} // psi