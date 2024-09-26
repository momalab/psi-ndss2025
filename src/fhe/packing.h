#pragma once

#include <cstdint>
#include <vector>
#include "crt.h"
#include "seal/seal.h"

namespace fhe
{

std::vector<uint64_t> packDecode(const seal::Plaintext & pt, const math::CrtParams & crt, const seal::BatchEncoder * encoder_ptr);

std::vector<uint64_t> packDecrypt(const seal::Ciphertext & ct, const math::CrtParams & crt, const seal::BatchEncoder * encoder_ptr, seal::Decryptor * decryptor_ptr);

void packEncode(seal::Plaintext & pt, const std::vector<uint64_t> & vs, const math::CrtParams & crt, const seal::BatchEncoder * encoder_ptr);

void packEncrypt(seal::Ciphertext & ct, const std::vector<uint64_t> & vs, const math::CrtParams & crt, const seal::BatchEncoder * encoder_ptr, const seal::Encryptor * encryptor_ptr);

void packEncrypt(std::vector<seal::Ciphertext> & vct, const std::vector<std::vector<uint64_t>> & vvs, const math::CrtParams & crt, const seal::BatchEncoder * encoder_ptr, const seal::Encryptor * encryptor_ptr);

void packEncrypt(std::vector<seal::Ciphertext> & vct, const std::vector<std::vector<uint64_t>> & vvs, const math::CrtParams & crt, const seal::BatchEncoder * encoder_ptr, const seal::Encryptor * encryptor_ptr, uint64_t num_threads);

void packEncrypt(std::vector<seal::Ciphertext> & vct, const std::vector<std::vector<uint64_t>> & vvs, const math::CrtParams & crt, const seal::BatchEncoder * encoder_ptr, const seal::Encryptor * encryptor_ptr, uint64_t step, uint64_t id);

} // fhe