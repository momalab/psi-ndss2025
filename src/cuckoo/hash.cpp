#include "hash.h"

#include <iostream>
#include <cstdlib>
#include <vector>

using namespace std;

namespace cuckoo
{

Hash::Hash(const std::vector<uint64_t> & coeffs, uint64_t mod, uint64_t prime, uint64_t seed)
{
    this->coeffs = coeffs;
    this->mod = mod;
    this->prime = prime;
    this->seed = seed;
}

uint64_t Hash::hash(uint64_t value) const
{
    // sufficiently uniform and uncorrelated
    return (((coeffs[3] * (value ^ seed) + coeffs[2]) % prime) * coeffs[1] + coeffs[0]) % mod;
}

uint64_t Hash::quickHash(uint64_t value) const
{
    // highly uniform, but not uncorrelated
    return ((value ^ seed) * coeffs[1] + coeffs[0]) % mod;
}

istream & operator>>(istream & is, Hash & hash)
{
    hash.coeffs.clear();
    uint64_t size;
    is >> size;
    for (uint64_t coeff; size-- && is >> coeff; hash.coeffs.push_back(coeff)); 
    is >> hash.mod >> hash.prime >> hash.seed;
    return is;
}

ostream & operator<<(ostream & os, const Hash & hash)
{
    if (hash.coeffs.empty()) return os;
    os << hash.coeffs.size();
    for (const auto & coeff : hash.coeffs) os << ' ' << coeff;
    os << ' ' << hash.mod << ' ' << hash.prime << ' ' << hash.seed;
    return os;
}

} // cuckoo
