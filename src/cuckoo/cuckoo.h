#pragma once

#include <cstdlib>
#include <vector>
#include "hash.h"

namespace cuckoo
{

class Cuckoo
{
    private:
        std::vector<Hash> hashes;
        std::vector<uint64_t> table;
        uint64_t max_data;
        uint64_t invalid_data;
        uint64_t num_hashes;
        uint64_t threshold;

    public:
        Cuckoo(){}
        Cuckoo(uint64_t num_hashes, uint64_t table_size, uint64_t max_data, uint64_t threshold);

        void insert(uint64_t value);

};

} // cuckoo
