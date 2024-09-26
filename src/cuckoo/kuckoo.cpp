#include "kuckoo.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <vector>
#include "math.h"
#include "prime.h"

using namespace std;

namespace cuckoo
{

// const uint64_t NUM_MUTEXES = 1<<8;
// const uint64_t MUTEX_MASK = NUM_MUTEXES - 1;
// vector<mutex> mtx (NUM_MUTEXES);
// vector<atomic_flag> locks(NUM_MUTEXES);
// vector<uint64_t> locks(NUM_MUTEXES, 0);

Kuckoo::Kuckoo(uint64_t num_hashes, uint64_t table_size, uint64_t max_data, uint64_t threshold, uint64_t num_tables)
{
    random_device rd;
    mt19937 gen(rd());

    uint64_t min_value = (table_size * table_size);   
    uniform_int_distribution<uint64_t> dist_table(0, table_size-1);
    uniform_int_distribution<uint64_t> dist_seeds(0, max_data);

    // create hash functions for Cuckoo hashing
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

    // create hash for function g(x)
    {
        uniform_int_distribution<uint64_t> dist_map(0, num_tables-1);
        uint64_t seed  = dist_seeds(gen);
        uint64_t mod   = num_tables;
        uint64_t prime = mod; // not actually prime in this case
        uint64_t c0    = dist_map(gen);
        uint64_t c1    = math::generatePrime(dist_map(gen));
        uint64_t c2    = 0;
        uint64_t c3    = 1;

        // c1 should not be a factor of num_tables
        while (num_tables % c1 == 0)
            c1 = math::generatePrime(c1);

        vector<uint64_t> coeffs { c0,c1,c2,c3 };
        this->g = Hash(coeffs, mod, prime, seed);
    }

    this->size_right = math::clog2(max_data+1ULL) - math::flog2(table_size);
    this->mask_right = math::shiftLeft(1ULL, size_right) - 1ULL;
    this->max_data = max_data & mask_right;
    this->invalid_data = this->max_data + 1ULL;
    this->num_hashes = num_hashes;
    this->threshold = threshold;
    this->table_hashes = vector<vector<uint64_t>>(num_tables, vector<uint64_t>(table_size, num_hashes));
    this->table_values = vector<vector<uint64_t>>(num_tables, vector<uint64_t>(table_size, invalid_data)); // initialize with max_data+1 (invalid value)
}

Kuckoo::Kuckoo(const KuckooParameters & params)
{
    tie(g, hashes, max_data, invalid_data, num_hashes, threshold, size_right, mask_right) = params;
}

KuckooIndices Kuckoo::getIndices(uint64_t value) const
{
    uint64_t x_l = value >> size_right;
    uint64_t x_r = value & mask_right;

    // map value to a table
    uint64_t table_index = g.quickHash(value);

    vector<uint64_t> indices(num_hashes);
    for (uint64_t i = 0; i < num_hashes; i++)
        indices[i] = x_l ^ hashes[i].hash(x_r);

    return make_tuple(x_r, table_index, indices);
}

uint64_t Kuckoo::getNumHashes() const
{
    return num_hashes;
}

KuckooParameters Kuckoo::getParameters() const
{
    return make_tuple(g, hashes, max_data, invalid_data, num_hashes, threshold, size_right, mask_right);
}

const vector<vector<uint64_t>> & Kuckoo::getTable() const
{
    return table_values;
}

void Kuckoo::insert(uint64_t value)
{
    // map value to a table
    uint64_t table_index = g.quickHash(value);
    
    // break value into left and right parts
    uint64_t x_l = value >> size_right;
    uint64_t x_r = value & mask_right;

    // cascade insertion if the slot is occupied
    uint64_t prev_hash_index = num_hashes;
    for (uint64_t i = 0; (x_r != invalid_data) && (i < threshold); i++)
    {
        uint64_t hash_index;
        do { hash_index = rand() % num_hashes; } // select a random hash function
        while (hash_index == prev_hash_index); // ensure that the same hash function is not selected twice
        prev_hash_index = hash_index;

        uint64_t bin_index = x_l ^ hashes[hash_index].hash(x_r); // index in the hash table given by x_l ^ H[i](x_r)
        swap(table_hashes[table_index][bin_index], prev_hash_index); // insert hash_index into the table
        swap(table_values[table_index][bin_index], x_r); // insert x_r into the table

        if (x_r != invalid_data)
            x_l = bin_index ^ hashes[prev_hash_index].hash(x_r); // recover x_l
    }

    if (x_r != invalid_data)
        throw runtime_error("Cuckoo insertion failed");
}

void Kuckoo::insert(const vector<uint64_t> & set)
{
    for (uint64_t value : set)
       insert(value);
}

void Kuckoo::insert(const vector<uint64_t> & set, uint64_t num_threads)
{
    num_threads = min(num_threads, table_values.size());
    vector<thread> threads(num_threads);
    vector<bool> failure(num_threads, false);
    
    for (uint64_t t = 0; t < num_threads; t++)
    {
        threads[t] = thread
        ([
            this, t, &failure, &set
        ]()
        {
            for (uint64_t j = t; j < set.size(); j++)
            {
                // uint64_t u = t+1;
                uint64_t value = set[j];

                // map value to a table
                uint64_t table_index = g.quickHash(value);
                if (table_index != t) continue;
                
                // break value into left and right parts
                uint64_t x_l = value >> size_right;
                uint64_t x_r = value & mask_right;

                // cascade insertion if the slot is occupied
                uint64_t prev_hash_index = num_hashes;
                for (uint64_t i = 0; (x_r != invalid_data) && (i < threshold); i++)
                {
                    uint64_t hash_index;
                    do { hash_index = rand() % num_hashes; } // select a random hash function
                    while (hash_index == prev_hash_index); // ensure that the same hash function is not selected twice
                    prev_hash_index = hash_index;
                    uint64_t bin_index = x_l ^ hashes[hash_index].hash(x_r); // index in the hash table given by x_l ^ H[i](x_r)
                    // uint64_t mtx_index = bin_index & MUTEX_MASK;
                    // mtx[mtx_index].lock();
                    // while (locks[mtx_index].test_and_set(memory_order_acquire)) this_thread::yield();
                    // while ((!locks[mtx_index]) && (locks[mtx_index] != u))
                    // {
                    //     if (!locks[mtx_index]) locks[mtx_index] = u;
                    //     else this_thread::yield();
                    // }
                    swap(table_hashes[table_index][bin_index], prev_hash_index); // insert hash_index into the table
                    swap(table_values[table_index][bin_index], x_r); // insert x_r into the table
                    // mtx[mtx_index].unlock();
                    // locks[mtx_index].clear(memory_order_release);
                    // locks[mtx_index] = 0;

                    if (x_r != invalid_data)
                        x_l = bin_index ^ hashes[prev_hash_index].hash(x_r); // recover x_l
                }

                if (x_r != invalid_data)
                {
                    failure[t] = true;
                    return;
                }
            }
        });
    }
    for (thread & t : threads) t.join();
    for (bool f : failure) 
        if (f) throw runtime_error("Cuckoo insertion failed");
}

istream & operator>>(istream & is, Kuckoo & cuckoo)
{
    is >> cuckoo.max_data;
    is >> cuckoo.invalid_data;
    is >> cuckoo.num_hashes;
    is >> cuckoo.threshold;
    is >> cuckoo.size_right;
    is >> cuckoo.mask_right;
    is >> cuckoo.g;
    for (uint64_t i=0; i<cuckoo.num_hashes; i++)
    {
        Hash hash;
        is >> hash;
        cuckoo.hashes.push_back(hash);
    }
    return is;
}

ostream & operator<<(ostream & os, const Kuckoo & cuckoo)
{
    os << cuckoo.max_data << ' ';
    os << cuckoo.invalid_data << ' ';
    os << cuckoo.num_hashes << ' ';
    os << cuckoo.threshold << ' ';
    os << cuckoo.size_right << ' ';
    os << cuckoo.mask_right << '\n';
    os << cuckoo.g << '\n';
    for (const Hash & hash : cuckoo.hashes) os << hash << '\n';
    return os;
}

} // cuckoo