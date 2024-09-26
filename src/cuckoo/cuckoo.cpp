#include "cuckoo.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include "math.h"
#include "prime.h"

using namespace std;

namespace cuckoo
{

Cuckoo::Cuckoo(uint64_t num_hashes, uint64_t table_size, uint64_t max_data, uint64_t threshold)
{
    random_device rd;
    mt19937 gen(rd());

    uint64_t min_value = (table_size * table_size);   
    uniform_int_distribution<uint64_t> dist_table(0, table_size-1);
    uniform_int_distribution<uint64_t> dist_seeds(0, max_data);

    vector<uint64_t> primes;
    for (uint64_t i = 0; i < num_hashes; i++)
    {
        // generate coprimes
        uint64_t prime;
        do
        {
            prime = math::generatePrime(min_value + dist_table(gen));
        } while (find(primes.begin(), primes.end(), prime) != primes.end());
        primes.push_back(prime);

        // generate hash parameters
        uint64_t seed = dist_seeds(gen);
        uniform_int_distribution<uint64_t> dist_prime(0, prime-1);
        uint64_t c0 = dist_table(gen);
        uint64_t c1;
        uint64_t c2 = math::generatePrime(dist_prime(gen));
        uint64_t c3 = dist_prime(gen);

        // c1 should not be a factor of table_size
        do { c1 = math::generatePrime(dist_table(gen)); }
        while (table_size % c1 == 0);

        vector<uint64_t> coeffs { c0,c1,c2,c3 };

        hashes.push_back(Hash(coeffs, table_size, prime, seed));
    }

    this->max_data = max_data;
    this->invalid_data = max_data+1;
    this->num_hashes = num_hashes;
    this->threshold = threshold;
    table.resize(table_size, invalid_data); // initialize with max_data+1 (invalid value)
}

void Cuckoo::insert(uint64_t value)
{
    // cascade insertion if the slot is occupied
    for (uint64_t i = 0; (value != invalid_data) && (i < threshold); i++)
    {
        uint64_t hash_index = rand() % num_hashes;
        uint64_t index = hashes[hash_index].hash(value);
        swap(table[index], value);
    }

    if (value != invalid_data)
        throw runtime_error("Cuckoo insertion failed");
}

} // cuckoo