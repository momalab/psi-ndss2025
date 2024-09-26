#pragma once

#include <cstdint>
#include <vector>

namespace math
{

std::vector<uint64_t> randomVector(uint64_t num_entries, uint64_t min_value, uint64_t max_value);
std::vector<uint64_t> randomVector(uint64_t num_entries, uint64_t min_value, uint64_t max_value, const std::vector<uint64_t> & moduli);

} // math