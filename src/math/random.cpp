#include "random.h"

#include <cstdint>
#include <random>
#include <vector>

using namespace std;

namespace math
{

vector<uint64_t> randomVector(uint64_t num_entries, uint64_t min_value, uint64_t max_value)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint64_t> dist(min_value, max_value);

    vector<uint64_t> result(num_entries);
    for (uint64_t i = 0; i < num_entries; i++)
        result[i] = dist(gen);

    return result;
}

vector<uint64_t> randomVector(uint64_t num_entries, uint64_t min_value, uint64_t max_value, const vector<uint64_t> & moduli)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint64_t> dist(min_value, max_value);

    vector<uint64_t> result(num_entries);
    for (uint64_t i = 0; i < num_entries; i++)
    {
        result[i] = dist(gen);
        for (const auto & modulus : moduli)
            if (result[i] % modulus == 0) { i--; break; }
    }

    return result;
}

} // math