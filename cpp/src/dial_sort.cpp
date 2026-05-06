#include "algorithms.hpp"

#include <cstddef>
#include <vector>

namespace dialsort {

void dialSort(std::vector<int32_t>& a, int32_t U) {
    const std::size_t n = a.size();
    std::vector<std::vector<int32_t>> buckets(static_cast<std::size_t>(U));

    for (std::size_t i = 0; i < n; ++i) {
        buckets[static_cast<std::size_t>(a[i])].push_back(a[i]);
    }

    std::size_t k = 0;
    for (std::size_t b = 0; b < static_cast<std::size_t>(U); ++b) {
        const auto& bucket = buckets[b];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            a[k++] = bucket[i];
        }
    }
}

std::size_t dialEstimatedBytes(std::size_t n, int32_t U) {
    // Empty std::vector is ~24 bytes on 64-bit; each pushed int adds ~4 bytes
    // plus vector growth overhead. Conservative: 32 B per bucket header + 12 B per element.
    return static_cast<std::size_t>(U) * 32 + n * 12;
}

bool dialMemorySafe(std::size_t n, int32_t U) {
    constexpr std::size_t MAX_BYTES = 500ull * 1024 * 1024;  // 500 MB
    constexpr int32_t MAX_U = 50'000'000;
    if (U > MAX_U) return false;
    return dialEstimatedBytes(n, U) <= MAX_BYTES;
}

}  // namespace dialsort
