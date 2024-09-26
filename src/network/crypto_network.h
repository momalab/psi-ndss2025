#pragma once

#include <tuple>
#include <vector>
#include "kuckoo.h"
#include "seal/seal.h"
#include "socket.h"

namespace network
{

std::vector<std::vector<seal::Ciphertext>> receiveCiphertexts(network::Socket & socket, const seal::SEALContext * context_ptr);

seal::GaloisKeys * receiveGaloisKeys(network::Socket & socket, const seal::SEALContext * context_ptr);

seal::RelinKeys * receiveRelinKeys(network::Socket & socket, const seal::SEALContext * context_ptr);

std::tuple<cuckoo::Kuckoo, std::vector<seal::Ciphertext>> receiveTable(network::Socket & socket, const seal::SEALContext * context_ptr);

void sendCiphertexts(network::Socket & socket, const std::vector<std::vector<seal::Ciphertext>> & cts);

void sendGaloisKeys(network::Socket & socket, const seal::GaloisKeys * galoiskeys_ptr);

void sendRelinKeys(network::Socket & socket, const seal::RelinKeys * relinkeys_ptr);

void sendTable(network::Socket & socket, const cuckoo::Kuckoo & cuckoo, const std::vector<seal::Ciphertext> & table);

} // network