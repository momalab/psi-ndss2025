// This program is to evalute the protocol's performance only
// For real-world deploying, use 'receiver_setup.cpp', 'sender_setup.cpp',
// 'receiver_intersect.cpp', and 'sender_intersect.cpp' instead

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "bfv.h"
#include "crt.h"
#include "kuckoo.h"
#include "math.h"
#include "packing.h"
#include "party.h"
#include "psi.h"
#include "seal/seal.h"

using namespace cuckoo;
using namespace fhe;
using namespace math;
using namespace psi;
using namespace seal;
using namespace std;
using namespace std::chrono;

using TimeUnit = milliseconds;
const string time_unit = "ms";

int main(int argc, char * argv[])
try
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <mode> <log2|X|> <|Y|> <m> <threads>" << endl;
        cerr << "mode: 0 (Fast Setup), 1 (Fast Intersection)" << endl;
        cerr << "log2|X|: log2 of the size of the Sender's set (default: 20)" << endl;
        cerr << "|Y|: size of the Receiver's set (default: 4)" << endl;
        cerr << "m: number of recurrencies (default: 1)" << endl;
        cerr << "threads: number of threads (default: 4)" << endl;
        return 1;
    }
    bool mode = stoi(argv[1]);
    uint64_t log2X = argc > 2 ? stoi(argv[2]) : 20;
    uint64_t sizeY = argc > 3 ? stoi(argv[3]) : 4;
    uint64_t m = argc > 4 ? stoi(argv[4]) : 1;
    uint64_t num_threads = argc > 5 ? stoi(argv[5]) : 4;

    /* Begin of parameters */

    // (Leveled) Fully Homomorphic Encryption parameters
    
    // Sender's parameters
    uint64_t sender_n = 1 << 12; vector<int> sender_logqi{27, 27, 27, 28};

    // Receiver's parameters
    uint64_t receiver_n = 1 << 12; vector<int> receiver_logqi{27, 27, 27, 28};

    // Shared parameters
    auto ti = mode ? vector<uint64_t>{40961} : vector<uint64_t>{40961, 65537};

    // CRT parameters
    auto crt = crtParams(ti);
    uint64_t k = ti.size();

    // Timers
    time_point<high_resolution_clock> start, end;
    uint64_t time_span;

    // Cuckoo hashing parameters (as per Section 6.A in the paper)
    uint64_t num_hashes = 4;
    uint64_t num_tables = k;
    uint64_t table_size = 1 << (log2X - (num_tables-1));
    uint64_t max_depth = 1<<10;
    double load_factor = mode ? 0.86 : 0.87;
    
    // Partitioning parameters
    uint64_t sender_eta = mode ? 0 : 1; // [0,h-1], where 0 means full multiplication, h-1 means full partitioning
    uint64_t receiver_eta = mode ? 0 : 1; // [0, h-1-sender_eta], where 0 means full multiplication, h-1-sender_eta means full partitioning

    uint64_t time_sender_pre = 0;
    uint64_t time_sender = 0;
    uint64_t time_receiver = 0;

    // First, we generate the Sender's set and the Receiver's sets to evaluate the protocol

    // Sender's set
    cout << "Generating Sender's set..." << flush;
    uint64_t sender_set_size = load_factor * table_size;
    uint64_t bitsize = 32;
    uint64_t max_data = shiftLeft(1ULL, bitsize) - 1ULL;
    Party sender(sender_set_size, bitsize);
    cout << "done." << endl;

    // print Sender's set -- commented out for large sets
    // const auto & sender_set = sender.getSet();
    // cout << "Sender's set:";
    // for (auto & value : sender_set) cout << " " << value;
    // cout << endl;

    // Receiver's sets
    cout << "Generating Receiver's sets..." << flush;
    // Create the Receiver's set from the Sender's set so intersections exist
    // This has no impact in performance, only in the size of the intersection
    // If we use random numbers, the intersection is likely to be empty
    vector<Party> receivers(m);
    for (uint64_t i = 0; i < m; i++)
        receivers[i] = Party(sizeY, bitsize, sender.getSet(), 0.5); // The last parameter indicates the probability of an entry being picked from the Sender's set
    cout << "done." << endl;

    // print Receiver's sets
    // for (uint64_t i = 0; i < receivers.size(); i++)
    // {
    //     const auto & receiver_set = receivers[i].getSet();
    //     cout << "Receiver's set " << i << ":";
    //     for (auto & value : receiver_set) cout << " " << value;
    //     cout << endl;
    // }

    /* End of parameters */

    /* Begin of setup */

    // Generate Sender's keys
    cout << "Generating Sender's keys and evaluator..." << flush;
    SEALContext* sender_context_ptr;
    do { sender_context_ptr = instantiateEncryptionScheme(sender_n, sender_logqi, ti); }
    while (!validKeys(sender_context_ptr));
    auto [sender_secret_key_ptr, sender_relinkeys_ptr, sender_galoiskeys_ptr] = generateKeys(sender_context_ptr, false);
    auto [sender_encoder_ptr, sender_evaluator_ptr] = generateEvaluator(sender_context_ptr);
    auto sender_encryptor_ptr = new Encryptor(*sender_context_ptr, *sender_secret_key_ptr);
    auto sender_decryptor_ptr = new Decryptor(*sender_context_ptr, *sender_secret_key_ptr);
    cout << "done." << endl;

    // Generate Receiver's keys
    cout << "Generating Receiver's keys and evaluator..." << flush;
    SEALContext* receiver_context_ptr;
    do { receiver_context_ptr = instantiateEncryptionScheme(receiver_n, receiver_logqi, ti); }
    while (!validKeys(receiver_context_ptr));
    auto [receiver_secret_key_ptr, receiver_relinkeys_ptr, receiver_galoiskeys_ptr] = generateKeys(receiver_context_ptr);
    auto [receiver_encoder_ptr, receiver_evaluator_ptr] = generateEvaluator(receiver_context_ptr);
    auto receiver_encryptor_ptr = new Encryptor(*receiver_context_ptr, *receiver_secret_key_ptr);
    auto receiver_decryptor_ptr = new Decryptor(*receiver_context_ptr, *receiver_secret_key_ptr);
    cout << "done." << endl;

    /* End of setup */

    /* Begin of Cuckoo hashing */

    // k-table Cuckoo hashing with Permutation-based hashing
    cout << "Generating Cuckoo hash table..." << flush;
    start = high_resolution_clock::now();
    Kuckoo cuckoo(num_hashes, table_size, max_data, max_depth, num_tables);
    cuckoo.insert(sender.getSet());//, num_threads);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    Kuckoo cuckoo_params(cuckoo.getParameters()); // this is what Receiver can see
    uint64_t receiver_dummy = get<3>(cuckoo.getParameters())+2;
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_sender_pre += time_span;

    cout << "Cuckoo hash table size: " << cuckoo.getTable().size() << " x " << cuckoo.getTable()[0].size() << endl;

    /* End of Cuckoo hashing */

    /* Begin of set encryption */

    // Encode and Encrypt Cuckoo hash table
    cout << "Encrypting Cuckoo hash table..." << flush;
    start = high_resolution_clock::now();
    vector<Ciphertext> encrypted_table;
    packEncrypt(encrypted_table, cuckoo.getTable(), crt, sender_encoder_ptr, sender_encryptor_ptr, num_threads);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_sender_pre += time_span;

    cout << "Encrypted table size: " << encrypted_table.size() << endl;
    {
        ofstream fout("encrypted_table.tmp");
        for (auto & ct : encrypted_table)
            ct.save(fout);
    }

    /* End of set encryption */


    // For each Receiver's set
    for (uint64_t idx=0; idx < receivers.size(); idx++)
    {
        /* Begin of set intersection */
        uint64_t ch_idx0 = idx / 26;
        uint64_t ch_idx1 = idx % 26;
        auto tag = string(1, char('A' + ch_idx0)) + string(1, char('A' + ch_idx1));
        auto & receiver = receivers[idx];

        cout << "Computing intersection..." << flush;
        start = high_resolution_clock::now();
        vector<vector<Ciphertext>> results, randoms;
        computeIntersection
        (
            results, randoms, receiver, cuckoo_params, encrypted_table, crt, sender_eta,
            sender_context_ptr, sender_encoder_ptr, sender_evaluator_ptr, sender_relinkeys_ptr,
            receiver_encoder_ptr, receiver_encryptor_ptr, receiver_dummy, num_threads
        );
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_receiver += time_span;

        // save results and randoms
        for (uint64_t i = 0; i < results.size(); i++)
        {
            for (uint64_t j = 0; j < results[i].size(); j++)
            {
                {
                    string filename = tag + "_D_" + to_string(i) + "_" + to_string(j) + ".tmp";
                    ofstream fout(filename);
                    results[i][j].save(fout);
                }
                {
                    string filename = tag + "_R_" + to_string(i) + "_" + to_string(j) + ".tmp";
                    ofstream fout(filename);
                    randoms[i][j].save(fout);
                }
            }
        }

        /* End of set intersection */

        /* Begin of decryption */
        
        cout << "Decrypting results..." << flush;
        start = high_resolution_clock::now();
        vector<vector<Ciphertext>> finals;
        recrypt
        (
            finals, results, randoms, crt, receiver_eta, sender_encoder_ptr, sender_decryptor_ptr,
            receiver_context_ptr, receiver_encoder_ptr, receiver_evaluator_ptr, receiver_relinkeys_ptr,
            receiver_galoiskeys_ptr, num_threads
        );
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_sender += time_span;

        // save finals
        for (uint64_t i = 0; i < finals.size(); i++)
        {
            for (uint64_t j = 0; j < finals[i].size(); j++)
            {
                string filename = tag + "_E_" + to_string(i) + "_" + to_string(j) + ".tmp";
                ofstream fout(filename);
                finals[i][j].save(fout);
            }
        }

        /* End of decryption */

        /* Begin of result */

        cout << "Decrypting intersection..." << flush;
        start = high_resolution_clock::now();
        auto intersection = decryptIntersection(finals, receiver, crt, receiver_encoder_ptr, receiver_decryptor_ptr, num_threads);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_receiver += time_span;

        cout << "Intersection size: " << intersection.size() << endl;
        // cout << "Intersection:";
        // for (auto & value : intersection) cout << " " << value;
        // cout << endl;

        /* End of result */
    }
    // write time_sender and time_receiver to file
    {
        ofstream fout("runtime.log");
        fout << time_sender_pre << endl;
        fout << time_sender << endl;
        fout << time_receiver << endl;
    }
} catch (const exception & e)
{
    cerr << e.what() << endl;
    return 1;
} catch (const char * e)
{
    cerr << e << endl;
    return 1;
} catch (...)
{
    cerr << "Unknown exception" << endl;
    return 1;
}