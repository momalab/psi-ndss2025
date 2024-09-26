#include "crypto_io.h"

#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include "kuckoo.h"
#include "seal/seal.h"

using namespace cuckoo;
using namespace seal;
using namespace std;

namespace io
{

GaloisKeys * loadGaloisKeys(const string & filename, const SEALContext * context_ptr)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";
    GaloisKeys * galoiskeys_ptr = new GaloisKeys();
    galoiskeys_ptr->load(*context_ptr, file);
    return galoiskeys_ptr;
}

RelinKeys * loadRelinKeys(const string & filename, const SEALContext * context_ptr)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";
    RelinKeys * relinkeys_ptr = new RelinKeys();
    relinkeys_ptr->load(*context_ptr, file);
    return relinkeys_ptr;
}

SecretKey * loadSecretKey(const string & filename, const SEALContext * context_ptr)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";
    SecretKey * secret_key_ptr = new SecretKey();
    secret_key_ptr->load(*context_ptr, file);
    return secret_key_ptr;
}

tuple<Kuckoo, vector<Ciphertext>> loadTable(const string & filename, const SEALContext * context_ptr)
{
    // Load table parameters
    ifstream file_params(filename + ".params");
    if (!file_params.is_open()) throw "Could not open file '" + filename + ".params";
    Kuckoo cuckoo;
    file_params >> cuckoo;

    // Load number of ciphertexts
    ifstream file_size(filename + ".size");
    if (!file_size.is_open()) throw "Could not open file '" + filename + ".size";
    uint64_t size;
    file_size >> size;

    // Load table ciphertexts (one ciphertext per file)
    vector<Ciphertext> table;
    {
        for (uint64_t i = 0; i < size; ++i)
        {
            ifstream file(filename + "_" + to_string(i) + ".ct", ios::binary);
            if (!file.is_open()) throw "Could not open file '" + filename + "." + to_string(i) + "'";
            Ciphertext ct;
            ct.load(*context_ptr, file);
            table.push_back(ct);
        }
    }

    return {cuckoo, table};
}

void saveGaloisKeys(const string & filename, const GaloisKeys * galoiskeys_ptr)
{
    ofstream file(filename, ios::binary);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";

    galoiskeys_ptr->save(file);
}

void saveRelinKeys(const string & filename, const RelinKeys * relinkeys_ptr)
{
    ofstream file(filename, ios::binary);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";

    relinkeys_ptr->save(file);
}

void saveSecretKey(const string & filename, const SecretKey * secret_key_ptr)
{
    ofstream file(filename, ios::binary);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";

    secret_key_ptr->save(file);
}

void saveTable(const string & filename, const Kuckoo & cuckoo, const vector<Ciphertext> & table)
{
    // Save table parameters
    ofstream file_params(filename + ".params");
    if (!file_params.is_open()) throw "Could not open file '" + filename + ".params";
    file_params << cuckoo;

    // Save number of ciphertexts
    ofstream file_size(filename + ".size");
    if (!file_size.is_open()) throw "Could not open file '" + filename + ".size";
    file_size << table.size();

    // Save table ciphertexts (one ciphertext per file)
    {
        for (uint64_t i = 0; i < table.size(); ++i)
        {
            ofstream file(filename + "_" + to_string(i) + ".ct", ios::binary);
            if (!file.is_open()) throw "Could not open file '" + filename + "." + to_string(i) + "'";
            table[i].save(file);
        }
    }
}

} // io