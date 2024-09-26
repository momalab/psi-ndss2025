#pragma once

#include <iostream>
#include <cstdint>
#include <vector>

namespace cuckoo
{

class Hash
{
    private:
        std::vector<uint64_t> coeffs;
        uint64_t mod;
        uint64_t prime;
        uint64_t seed;

    public:
        Hash(){}
        Hash(const std::vector<uint64_t> & coeffs, uint64_t mod, uint64_t prime, uint64_t seed);

        uint64_t hash(uint64_t value) const;
        uint64_t quickHash(uint64_t value) const;

        friend std::istream & operator>>(std::istream & is, Hash & hash);
        friend std::ostream & operator<<(std::ostream & os, const Hash & hash);
};

} // cuckoo