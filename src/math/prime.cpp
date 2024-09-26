#include "prime.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <gmp.h>
#include <vector>
#include "math.h"

using namespace std;

namespace math
{

// check if all items in the vector are coprimes
bool areCoprime(const vector<uint64_t> & v)
{
    for (size_t i = 0; i < v.size(); i++)
        for (size_t j = i + 1; j < v.size(); j++)
            if (gcd(v[i], v[j]) != 1) return false;
    return true;   
}

// greatest common divisor
uint64_t gcd(uint64_t a, uint64_t b)
{
    while (b != 0)
    {
        uint64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// generate a prime number with a given number of bits
uint64_t generatePrime(uint64_t min_value)
{
    mpz_t candidate;
    mpz_init_set_ui(candidate, min_value);
    mpz_nextprime(candidate, candidate);
    uint64_t prime = mpz_get_ui(candidate);
    mpz_clear(candidate);
    return prime;

    // uint64_t candidate = min_value;
    // if ((candidate & 1) == 0) candidate++;
    // for (; !isPrime(candidate, 5); candidate += 2);
    // return candidate;
}

// check if a number is prime
uint64_t isPrime(uint64_t n, int k)
{
    if (n <= 1 || n == 4) return false;
    if (n <= 3) return true;

    uint64_t d = n - 1;
    while ((d & 1) == 0) d >>= 1;

    for (int i = 0; i < k; i++)
        if (!millerRabinTest(n, d)) return false;

    return true;
}

// Miller-Rabin primality test
bool millerRabinTest(uint64_t n, uint64_t d)
{
    uint64_t a = 2 + rand() % (n - 4);
    uint64_t x = powm(a, d, n);

    if (x == 1 || x == n - 1) return true;

    while (d != n - 1)
    {
        x = (x * x) % n;
        d <<= 1;
        if (x == 1) return false;
        if (x == n - 1) return true;
    }

    return false;
}

} // math