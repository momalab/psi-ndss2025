#include "bfv.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <tuple>
#include <vector>
#include "seal/seal.h"

using namespace seal;
using namespace std;

namespace fhe
{

EvalTuple generateEvaluator(const SEALContext * context_ptr)
{
    const auto & context = *context_ptr;
    auto encoder_ptr = new BatchEncoder(context);
    auto evaluator_ptr = new Evaluator(context);
    return EvalTuple(encoder_ptr, evaluator_ptr);
}

KeysTuple generateKeys(const SEALContext * context_ptr, bool galois)
{
    const auto & context = *context_ptr;
    KeyGenerator keygen(context);
    auto secret_key_ptr = new SecretKey(keygen.secret_key());
    auto relinkeys_ptr = new RelinKeys();
    keygen.create_relin_keys(*relinkeys_ptr);
    GaloisKeys * galoiskeys_ptr = nullptr;
    if (galois)
    {
        galoiskeys_ptr = new GaloisKeys();
        keygen.create_galois_keys(*galoiskeys_ptr);
    }
    return KeysTuple(secret_key_ptr, relinkeys_ptr, galoiskeys_ptr);
}

SEALContext * instantiateEncryptionScheme(uint64_t n, const std::vector<int> & logqi, const std::vector<uint64_t> & ti)
{
    EncryptionParameters params(scheme_type::bfv);
    params.set_poly_modulus_degree(n);
    if (!logqi.empty()) params.set_coeff_modulus(CoeffModulus::Create(n,logqi));
    else params.set_coeff_modulus(CoeffModulus::BFVDefault(n));
    uint64_t t = 1;
    for (const auto e : ti) t *= e;
    params.set_plain_modulus(t);
    auto context_ptr = new SEALContext(params);
    return context_ptr;
}

void rotate(Ciphertext & ct, uint64_t steps, uint64_t n, const Evaluator * evaluator_ptr, const GaloisKeys * galoiskeys_ptr)
{
    n >>= 1; // n = n/2
    if (steps > n) evaluator_ptr->rotate_columns_inplace(ct, *galoiskeys_ptr);
    steps %= n;
    if (steps) evaluator_ptr->rotate_rows_inplace(ct, steps, *galoiskeys_ptr);
}

bool validKeys(const SEALContext * context_ptr)
{
    const auto & context = *context_ptr;
    try
    {
        BatchEncoder encoder(context);
        Plaintext pt;
        vector<uint64_t> vi = {0}, vo;
        encoder.encode(vi, pt);
        encoder.decode(pt, vo);
        if (vi[0] != vo[0]) return false;
    }
    catch (const std::invalid_argument& e) { return false; }
    catch (...) { return false; }
    return true;
}

} // fhe