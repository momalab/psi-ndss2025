#include "crt.h"

#include <cstdint>
#include <vector>
#include "math.h"

using namespace std;

namespace math
{

vector<uint64_t> crtDecode(const vector<uint64_t> & vpack, const CrtParams & crt)
{
    auto step = crt.mi.size();
    vector<uint64_t> vs(vpack.size() * step);
    for (uint64_t i=0; i<vpack.size(); i++)
        for (uint64_t j=0; j<step; j++) vs[i*step+j] = vpack[i] % crt.mi[j];
    return vs;
}

vector<uint64_t> crtEncode(const vector<uint64_t> & vs, const CrtParams & crt)
{
    auto step = crt.mi.size();
    auto vs_size = vs.size();
    if (vs_size % step) throw "Invalid number of CRT components";

    auto size = vs.size() / step;
    vector<uint64_t> vpack(size, 0);
    for (size_t i=0; i<size; i++)
    {
        for (size_t j=0; j<step; j++)
        {
            auto v = vs[i*step + j];
            uint64_t Mi = crt.Mi[j];
            uint64_t iMi = crt.iMi[j];
            vpack[i] += v*Mi*iMi;
        }
        vpack[i] %= crt.M;
    }
    return vpack;
}

CrtParams crtParams(const vector<uint64_t> & vt)
{
    CrtParams crt;
    crt.mi = vt;
    crt.M = 1;
    for (const auto mi : crt.mi) crt.M *= mi;
    for (const auto mi : crt.mi) crt.Mi.push_back(crt.M/mi);
    for (uint64_t i=0; i<crt.Mi.size(); i++) crt.iMi.push_back(modinv(crt.Mi[i],crt.mi[i]));
    return crt;
}

} // math