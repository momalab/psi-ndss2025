#pragma once

#include <cstdint>
#include <vector>
#include "crt.h"
#include "kuckoo.h"
#include "party.h"
#include "seal/seal.h"

namespace psi
{

void computeIntersection // single-thread
(
    std::vector<std::vector<seal::Ciphertext>> & results, // return masked intersection under Sender's key
    std::vector<std::vector<seal::Ciphertext>> & randoms,  // return random masks under Receiver's key
    const Party & receiver,
    const cuckoo::Kuckoo & cuckoo,
    const std::vector<seal::Ciphertext> & encrypted_table,
    const math::CrtParams & crt,
    uint64_t sender_eta,
    const seal::SEALContext * sender_context_ptr,
    const seal::BatchEncoder * sender_encoder_ptr,
    const seal::Evaluator * sender_evaluator_ptr,
    const seal::RelinKeys * sender_relinkeys_ptr,
    const seal::BatchEncoder * receiver_encoder_ptr,
    const seal::Encryptor * receiver_encryptor_ptr,
    uint64_t receiver_dummy
);

void computeIntersection // multi-thread
(
    std::vector<std::vector<seal::Ciphertext>> & results, // return masked intersection under Sender's key
    std::vector<std::vector<seal::Ciphertext>> & randoms,  // return random masks under Receiver's key
    const Party & receiver,
    const cuckoo::Kuckoo & cuckoo,
    const std::vector<seal::Ciphertext> & encrypted_table,
    const math::CrtParams & crt,
    uint64_t sender_eta,
    const seal::SEALContext * sender_context_ptr,
    const seal::BatchEncoder * sender_encoder_ptr,
    const seal::Evaluator * sender_evaluator_ptr,
    const seal::RelinKeys * sender_relinkeys_ptr,
    const seal::BatchEncoder * receiver_encoder_ptr,
    const seal::Encryptor * receiver_encryptor_ptr,
    uint64_t receiver_dummy,
    uint64_t num_threads
);

std::vector<uint64_t> decryptIntersection // single-thread
(
    const std::vector<std::vector<seal::Ciphertext>> & finals,
    const Party & receiver,
    const math::CrtParams & crt,
    const seal::BatchEncoder * receiver_encoder_ptr,
    seal::Decryptor * receiver_decryptor_ptr
);

std::vector<uint64_t> decryptIntersection // multi-thread
(
    const std::vector<std::vector<seal::Ciphertext>> & finals,
    const Party & receiver,
    const math::CrtParams & crt,
    const seal::BatchEncoder * receiver_encoder_ptr,
    seal::Decryptor * receiver_decryptor_ptr,
    uint64_t num_threads
);

void recrypt // single-thread
(
    std::vector<std::vector<seal::Ciphertext>> & finals, // return rotated intersection under Receiver's key
    const std::vector<std::vector<seal::Ciphertext>> & results,
    const std::vector<std::vector<seal::Ciphertext>> & randoms,
    const math::CrtParams & crt,
    uint64_t receiver_eta,
    const seal::BatchEncoder * sender_encoder_ptr,
    seal::Decryptor * sender_decryptor_ptr,
    const seal::SEALContext * receiver_context_ptr,
    const seal::BatchEncoder * receiver_encoder_ptr,
    const seal::Evaluator * receiver_evaluator_ptr,
    const seal::RelinKeys * receiver_relinkeys_ptr,
    const seal::GaloisKeys * receiver_galoiskeys_ptr
);

void recrypt // multi-thread
(
    std::vector<std::vector<seal::Ciphertext>> & finals, // return rotated intersection under Receiver's key
    const std::vector<std::vector<seal::Ciphertext>> & results,
    const std::vector<std::vector<seal::Ciphertext>> & randoms,
    const math::CrtParams & crt,
    uint64_t receiver_eta,
    const seal::BatchEncoder * sender_encoder_ptr,
    seal::Decryptor * sender_decryptor_ptr,
    const seal::SEALContext * receiver_context_ptr,
    const seal::BatchEncoder * receiver_encoder_ptr,
    const seal::Evaluator * receiver_evaluator_ptr,
    const seal::RelinKeys * receiver_relinkeys_ptr,
    const seal::GaloisKeys * receiver_galoiskeys_ptr,
    uint64_t num_threads
);

} // psi