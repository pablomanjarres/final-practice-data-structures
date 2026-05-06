#include "algorithms.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dialsort {

void radixSortLSD(std::vector<int32_t>& a) {
    const std::size_t n = a.size();
    if (n <= 1) return;

    std::vector<int32_t> buf(n);
    std::vector<int32_t>* src = &a;
    std::vector<int32_t>* dst = &buf;

    constexpr int K = 256;
    int count[K];

    for (int shift = 0; shift < 32; shift += 8) {
        std::fill(count, count + K, 0);

        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t b = (static_cast<std::uint32_t>((*src)[i]) >> shift) & 0xff;
            ++count[b];
        }
        for (int i = 1; i < K; ++i) count[i] += count[i - 1];

        // Right-to-left placement keeps the sort stable.
        for (std::size_t i = n; i-- > 0;) {
            const std::uint8_t b = (static_cast<std::uint32_t>((*src)[i]) >> shift) & 0xff;
            (*dst)[--count[b]] = (*src)[i];
        }
        std::swap(src, dst);
    }

    // Ensure the result lives in `a`. After 4 passes (even), src == &a already.
    if (src != &a) {
        a.swap(*src);
    }
}

}  // namespace dialsort
