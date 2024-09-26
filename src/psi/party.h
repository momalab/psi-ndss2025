#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace psi
{

class Party
{
    private:
        uint64_t bitsize;
        std::vector<uint64_t> set;

    public:
        Party();
        Party(const std::string & filename);
        Party(uint64_t num_entries, uint64_t bitsize);
        Party(uint64_t num_entries, uint64_t bitsize, const std::vector<uint64_t> & source_set, double source_probability = 0.5);

        uint64_t getBitSize() const;
        const std::vector<uint64_t> & getSet() const;
        void save(const std::string & filename) const;
};

} // psi