#include "math.h"

#include <cstdint>

using namespace std;

namespace math
{

const uint64_t max_shift = 31;

// ceil(log2(x))
uint64_t clog2(uint64_t x)
{
    uint64_t result = flog2(x);
    if (x > (uint64_t(1) << result)) result++;
    return result;
}

// floor(log2(x))
uint64_t flog2(uint64_t x)
{
    uint64_t result = 0;
    while (x >>= 1) result++;
    return result;
}

uint64_t modinv(uint64_t a, uint64_t m)
{
    a = a % m;
    for (uint64_t x=1; x<m; x++)
        if ((a*x) % m == 1) return x;
    return 1;
}

// modular exponentiation
uint64_t powm(uint64_t b, uint64_t e, uint64_t m)
{
    uint64_t result = 1;
    b %= m;
    while (e > 0)
    {
        if (e & 1) result = (result * b) % m;
        e >>= 1;
        b = (b * b) % m;
    }
    return result;
}

// shift left for any shifting amount
uint64_t shiftLeft(uint64_t x, uint64_t s)
{
    while (s > max_shift)
    {
        x <<= max_shift;
        s -= max_shift;
    }
    return x << s;
}

// shift right for any shifting amount
uint64_t shiftRight(uint64_t x, uint64_t s)
{
    while (s > max_shift)
    {
        x >>= max_shift;
        s -= max_shift;
    }
    return x >> s;
}

} // math