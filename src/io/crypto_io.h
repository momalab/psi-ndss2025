#pragma once

#include <string>
#include <tuple>
#include <vector>
#include "kuckoo.h"
#include "seal/seal.h"

namespace io
{

seal::GaloisKeys * loadGaloisKeys(const std::string & filename, const seal::SEALContext * context_ptr);

seal::RelinKeys * loadRelinKeys(const std::string & filename, const seal::SEALContext * context_ptr);

seal::SecretKey * loadSecretKey(const std::string & filename, const seal::SEALContext * context_ptr);

std::tuple<cuckoo::Kuckoo, std::vector<seal::Ciphertext>> loadTable(const std::string & filename, const seal::SEALContext * context_ptr);

void saveGaloisKeys(const std::string & filename, const seal::GaloisKeys * galoiskeys_ptr);

void saveRelinKeys(const std::string & filename, const seal::RelinKeys * relinkeys_ptr);

void saveSecretKey(const std::string & filename, const seal::SecretKey * secret_key_ptr);

void saveTable(const std::string & filename, const cuckoo::Kuckoo & cuckoo, const std::vector<seal::Ciphertext> & table);

} // io