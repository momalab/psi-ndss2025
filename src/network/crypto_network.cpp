#include "crypto_network.h"

#include <sstream>
#include <tuple>
#include <vector>
#include "kuckoo.h"
#include "seal/seal.h"
#include "socket.h"

using namespace cuckoo;
using namespace seal;
using namespace std;

namespace network
{

vector<vector<Ciphertext>> receiveCiphertexts(Socket & socket, const SEALContext * context_ptr)
{
    // Receive the dimensions of the vector of ciphertexts
    size_t n_rows, n_cols;
    socket.receive() >> n_rows >> n_cols;

    // Receive each vector of ciphertexts
    vector<vector<Ciphertext>> cts(n_rows, vector<Ciphertext>(n_cols));
    for (size_t i = 0; i < n_rows; i++)
    {
        for (size_t j = 0; j < n_cols; j++)
        {
            stringstream ss = socket.receive();
            cts[i][j].load(*context_ptr, ss);
        }
    }

    return cts;
}

GaloisKeys * receiveGaloisKeys(Socket & socket, const SEALContext * context_ptr)
{
    stringstream ss = socket.receive();
    GaloisKeys * galoiskeys_ptr = new GaloisKeys();
    galoiskeys_ptr->load(*context_ptr, ss);
    return galoiskeys_ptr;
}

RelinKeys * receiveRelinKeys(Socket & socket, const SEALContext * context_ptr)
{
    stringstream ss = socket.receive();
    RelinKeys * relinkeys_ptr = new RelinKeys();
    relinkeys_ptr->load(*context_ptr, ss);
    return relinkeys_ptr;
}

tuple<Kuckoo, vector<Ciphertext>> receiveTable(Socket & socket, const SEALContext * context_ptr)
{
    // Receive the table parameters
    Kuckoo cuckoo;
    socket.receive() >> cuckoo;

    // Receive the number of ciphertexts in the table
    size_t size;
    socket.receive() >> size;

    // Receive each ciphertext in the table
    vector<Ciphertext> table;
    for (size_t i = 0; i < size; i++)
    {
        stringstream ss = socket.receive();
        Ciphertext ct;
        ct.load(*context_ptr, ss);
        table.push_back(ct);
    }

    return { cuckoo, table };
}

void sendCiphertexts(Socket & socket, const vector<vector<Ciphertext>> & cts)
{
    if (cts.empty()) throw "Cannot send an empty vector of ciphertexts.";

    // Send the dimensions of the vector of ciphertexts
    {
        stringstream ss;
        ss << cts.size() << " " << cts[0].size();
        socket.send(ss);
    }

    // Send each vector of ciphertexts
    for (const auto & row : cts)
    {
        for (const auto & ct : row)
        {
            stringstream ss;
            ct.save(ss);
            socket.send(ss);
        }
    }
}

void sendGaloisKeys(Socket & socket, const GaloisKeys * galoiskeys_ptr)
{
    stringstream ss;
    galoiskeys_ptr->save(ss);
    socket.send(ss);
}

void sendRelinKeys(Socket & socket, const RelinKeys * relinkeys_ptr)
{
    stringstream ss;
    relinkeys_ptr->save(ss);
    ss.seekg(0, ios::end);
    socket.send(ss);
}

void sendTable(Socket & socket, const Kuckoo & cuckoo, const vector<Ciphertext> & table)
{
    // Send the table parameters
    {
        stringstream ss;
        ss << cuckoo;
        socket.send(ss);
    }

    // Send the number of ciphertexts in the table
    {
        stringstream ss;
        ss << table.size();
        socket.send(ss);
    }

    // Send each ciphertext in the table
    for (const auto & ct : table)
    {
        stringstream ss;
        ct.save(ss);
        socket.send(ss);
    }
}

} // network