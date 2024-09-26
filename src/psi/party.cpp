#include "party.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <random>
#include <vector>
#include <unordered_set>
#include "math.h"

using namespace std;

namespace psi
{

Party::Party() {}

Party::Party(const string & filename)
{
    ifstream file(filename);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";
    for (uint64_t value; file >> value;) set.push_back(value);
    uint64_t max_value = *max_element(set.begin(), set.end());
    this->bitsize = math::clog2(max_value);
}

Party::Party(uint64_t num_entries, uint64_t bitsize)
{
    random_device rd;
    mt19937 gen(rd());
    uint64_t max_value = math::shiftLeft(1ULL, bitsize) - 1ULL;
    uniform_int_distribution<uint64_t> dist(0ULL, max_value);

    std::unordered_set<uint64_t> unique_set;
    while (unique_set.size() < num_entries)
        unique_set.insert(dist(gen));

    this->set = vector<uint64_t>(unique_set.begin(), unique_set.end());
    this->bitsize = bitsize;
}

Party::Party(uint64_t num_entries, uint64_t bitsize, const vector<uint64_t> & source_set, double source_probability)
{
    random_device rd;
    mt19937 gen(rd());
    uint64_t max_value = math::shiftLeft(1ULL, bitsize) - 1ULL;
    uniform_int_distribution<uint64_t> dist(0ULL, max_value);
    uniform_int_distribution<uint64_t> dist_idx(0, source_set.size()-1);
    uniform_real_distribution<double> dist_prob(0.0, 1.0);

    std::unordered_set<uint64_t> unique_set;
    while (unique_set.size() < num_entries)
    {
        if (dist_prob(gen) <= source_probability)
            unique_set.insert(source_set[dist_idx(gen)]);
        else
            unique_set.insert(dist(gen));
    }

    this->set = vector<uint64_t>(unique_set.begin(), unique_set.end());
    this->bitsize = bitsize;
}

uint64_t Party::getBitSize() const
{
    return bitsize;
}

const vector<uint64_t> & Party::getSet() const
{
    return set;
}

void Party::save(const string & filename) const
{
    ofstream file(filename);
    if (!file.is_open()) throw "Could not open file '" + filename + "'";
    for (uint64_t value : set) file << value << endl;
}

} // psi