// k-table Cuckoo hashing with Permutation-based hashing

#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <tuple>
#include "hash.h"

namespace cuckoo
{

using KuckooIndices = std::tuple<uint64_t, uint64_t, std::vector<uint64_t>>;
using KuckooParameters = std::tuple<Hash, std::vector<Hash>, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;

class Kuckoo
{
    private:
        Hash g;
        std::vector<Hash> hashes;
        std::vector<std::vector<uint64_t>> table_hashes;
        std::vector<std::vector<uint64_t>> table_values;
        uint64_t max_data;
        uint64_t invalid_data;
        uint64_t num_hashes;
        uint64_t threshold;
        uint64_t size_right;
        uint64_t mask_right;

    public:
        Kuckoo(){}
        Kuckoo(uint64_t num_hashes, uint64_t table_size, uint64_t max_data, uint64_t threshold, uint64_t num_tables = 1);
        Kuckoo(const KuckooParameters & params);

        KuckooIndices getIndices(uint64_t value) const;
        uint64_t getNumHashes() const;
        KuckooParameters getParameters() const;
        const std::vector<std::vector<uint64_t>> & getTable() const;
        void insert(uint64_t value);
        void insert(const std::vector<uint64_t> & set);
        void insert(const std::vector<uint64_t> & set, uint64_t num_threads);

        friend std::istream & operator>>(std::istream & is, Kuckoo & cuckoo);
        friend std::ostream & operator<<(std::ostream & os, const Kuckoo & cuckoo);
};

} // cuckoo
