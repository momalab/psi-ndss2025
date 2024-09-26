#pragma once

#include <cstdint>
#include <vector>

namespace math
{

struct CrtParams
{
    uint64_t M;
    std::vector<uint64_t> mi;
    std::vector<uint64_t> Mi;
    std::vector<uint64_t> iMi;
};

std::vector<uint64_t> crtDecode(const std::vector<uint64_t> & vpack, const CrtParams & crt);

std::vector<uint64_t> crtEncode(const std::vector<uint64_t> & vs, const CrtParams & crt);

CrtParams crtParams(const std::vector<uint64_t> & vt);

} // math