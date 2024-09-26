#pragma once

#include <cstdint>
#include <vector>

namespace math
{

bool areCoprime(const std::vector<uint64_t> & v);
uint64_t gcd(uint64_t a, uint64_t b);
uint64_t generatePrime(uint64_t min_value);
uint64_t isPrime(uint64_t n, int k);
bool millerRabinTest(uint64_t n, uint64_t d);

} // math