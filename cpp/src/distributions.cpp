#include "distributions.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace dialsort {

const char* distName(Distribution d) {
    switch (d) {
        case Distribution::Uniform:   return "uniform";
        case Distribution::Sorted:    return "sorted";
        case Distribution::Reverse:   return "reverse";
        case Distribution::Gaussian:  return "gaussian";
        case Distribution::SparseDup: return "sparseDup";
    }
    return "unknown";
}

bool parseDistribution(const std::string& s, Distribution& out) {
    if (s == "uniform")   { out = Distribution::Uniform;   return true; }
    if (s == "sorted")    { out = Distribution::Sorted;    return true; }
    if (s == "reverse")   { out = Distribution::Reverse;   return true; }
    if (s == "gaussian")  { out = Distribution::Gaussian;  return true; }
    if (s == "sparseDup" || s == "sparsedup") {
        out = Distribution::SparseDup;
        return true;
    }
    return false;
}

std::vector<int32_t> generateInput(Distribution dist,
                                   std::size_t n,
                                   int32_t U,
                                   std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::vector<int32_t> out(n);

    switch (dist) {
        case Distribution::Uniform: {
            std::uniform_int_distribution<int32_t> d(0, U - 1);
            for (std::size_t i = 0; i < n; ++i) out[i] = d(rng);
            break;
        }
        case Distribution::Sorted: {
            for (std::size_t i = 0; i < n; ++i) {
                out[i] = static_cast<int32_t>(
                    static_cast<double>(i) / static_cast<double>(n) * U);
            }
            break;
        }
        case Distribution::Reverse: {
            for (std::size_t i = 0; i < n; ++i) {
                out[i] = static_cast<int32_t>(
                    static_cast<double>(n - 1 - i) / static_cast<double>(n) * U);
            }
            break;
        }
        case Distribution::Gaussian: {
            const double mu = U / 2.0;
            const double sigma = U / 6.0;
            std::normal_distribution<double> d(mu, sigma);
            for (std::size_t i = 0; i < n; ++i) {
                double v = d(rng);
                if (v < 0)        v = 0;
                else if (v >= U)  v = U - 1;
                out[i] = static_cast<int32_t>(v);
            }
            break;
        }
        case Distribution::SparseDup: {
            const int32_t Ueff = std::max(
                int32_t{2},
                std::min(U, static_cast<int32_t>(std::max<std::size_t>(2, n / 100))));
            std::uniform_int_distribution<int32_t> d(0, Ueff - 1);
            for (std::size_t i = 0; i < n; ++i) out[i] = d(rng);
            break;
        }
    }
    return out;
}

}  // namespace dialsort
