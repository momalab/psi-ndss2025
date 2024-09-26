#pragma once

#include <cstdint>
#include <tuple>
#include <vector>
#include "seal/seal.h"

namespace fhe
{

using KeysTuple = std::tuple<seal::SecretKey*, seal::RelinKeys*, seal::GaloisKeys*>;
using EvalTuple = std::tuple<seal::BatchEncoder*, seal::Evaluator*>;

EvalTuple generateEvaluator(const seal::SEALContext * context_ptr);

KeysTuple generateKeys(const seal::SEALContext * context_ptr, bool galois = true);

seal::SEALContext * instantiateEncryptionScheme(uint64_t n, const std::vector<int> & logqi, const std::vector<uint64_t> & ti);

void rotate(seal::Ciphertext & ct, uint64_t steps, uint64_t n, const seal::Evaluator * evaluator_ptr, const seal::GaloisKeys * galoiskeys_ptr);

bool validKeys(const seal::SEALContext * context_ptr);

} // fhe