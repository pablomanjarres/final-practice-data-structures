#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dialsort {

enum class Distribution {
    Uniform,
    Sorted,
    Reverse,
    Gaussian,
    SparseDup,
};

const char* distName(Distribution d);
bool parseDistribution(const std::string& s, Distribution& out);

std::vector<int32_t> generateInput(Distribution dist,
                                   std::size_t n,
                                   int32_t U,
                                   std::uint32_t seed);

}  // namespace dialsort
