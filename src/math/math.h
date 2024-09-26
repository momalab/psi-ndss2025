#pragma once

#include <cstdint>
#include <vector>

namespace math
{

uint64_t clog2(uint64_t x);
uint64_t flog2(uint64_t x);
uint64_t modinv(uint64_t a, uint64_t m);
uint64_t powm(uint64_t b, uint64_t e, uint64_t m);
uint64_t shiftLeft(uint64_t x, uint64_t s);
uint64_t shiftRight(uint64_t x, uint64_t s);
template <class T> T sum(const std::vector<T> & v);

template <class T>
T sum(const std::vector<T> & v)
{
    T s = 0;
    for (auto & x : v) s += x;
    return s;
}

} // math