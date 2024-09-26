#include "packing.h"

#include <cstdint>
#include <thread>
#include <vector>
#include "crt.h"
#include "seal/seal.h"

using namespace math;
using namespace seal;
using namespace std;

namespace fhe
{
    
vector<uint64_t> packDecode(const Plaintext & pt, const CrtParams & crt, const BatchEncoder * encoder_ptr)
{
    vector<uint64_t> vpack;
    encoder_ptr->decode(pt, vpack);
    return crtDecode(vpack, crt);
}

vector<uint64_t> packDecrypt(const Ciphertext & ct, const CrtParams & crt, const BatchEncoder * encoder_ptr, Decryptor * decryptor_ptr)
{
    Plaintext pt;
    decryptor_ptr->decrypt(ct, pt);
    return packDecode(pt, crt, encoder_ptr);
}

void packEncode(Plaintext & pt, const vector<uint64_t> & vs, const CrtParams & crt, const BatchEncoder * encoder_ptr)
{
    auto vpack = crtEncode(vs, crt);
    encoder_ptr->encode(vpack, pt);
}

void packEncrypt(Ciphertext & ct, const vector<uint64_t> & vs, const CrtParams & crt, const BatchEncoder * encoder_ptr, const Encryptor * encryptor_ptr)
{
    Plaintext pt;
    packEncode(pt, vs, crt, encoder_ptr);
    encryptor_ptr->encrypt_symmetric(pt, ct);
}

void packEncrypt(std::vector<seal::Ciphertext> & vct, const vector<vector<uint64_t>> & vvs, const CrtParams & crt, const BatchEncoder * encoder_ptr, const Encryptor * encryptor_ptr)
{
    uint64_t k = crt.mi.size();
    if (vvs.size() != k) throw "Invalid number of CRT components";

    uint64_t n = encoder_ptr->slot_count();
    uint64_t size_vs = vvs[0].size();
    uint64_t size_vct = size_vs / n + bool(size_vs % n);
    vct.resize(size_vct);
    for (uint64_t i=0; i<size_vct; i++)
    {
        vector<uint64_t> vs(k * n);
        uint64_t offset = i*n;
        uint64_t m = min(n, size_vs-offset);
        for (uint64_t j=0; j<m; j++)
        {
            for (uint64_t l=0; l<k; l++)
                vs[j*k+l] = vvs[l][offset+j];
        }
        packEncrypt(vct[i], vs, crt, encoder_ptr, encryptor_ptr);
    }
}

void packEncrypt(std::vector<seal::Ciphertext> & vct, const vector<vector<uint64_t>> & vvs, const CrtParams & crt, const BatchEncoder * encoder_ptr, const Encryptor * encryptor_ptr, uint64_t num_threads)
{
    uint64_t k = crt.mi.size();
    if (vvs.size() != k) throw "Invalid number of CRT components";

    uint64_t n = encoder_ptr->slot_count();
    uint64_t size_vs = vvs[0].size();
    uint64_t size_vct = size_vs / n + bool(size_vs % n);
    vct.resize(size_vct);
    num_threads = min(num_threads, size_vct);
    vector<thread> threads(num_threads);
    for (uint64_t t=0; t<num_threads; t++)
    {
        threads[t] = thread([t, num_threads, &vct, &vvs, &crt, encoder_ptr, encryptor_ptr, n, size_vs, size_vct, k]()
        {
            uint64_t kn = k * n;
            for (uint64_t i=t; i<size_vct; i+=num_threads)
            {
                vector<uint64_t> vs(kn);
                uint64_t offset = i*n;
                uint64_t m = min(n, size_vs-offset);
                for (uint64_t j=0; j<m; j++)
                {
                    for (uint64_t l=0; l<k; l++)
                        vs[j*k+l] = vvs[l][offset+j];
                }
                packEncrypt(vct[i], vs, crt, encoder_ptr, encryptor_ptr);
            }
        });
    }
    for (auto & thread : threads) thread.join();
}

} // fhe