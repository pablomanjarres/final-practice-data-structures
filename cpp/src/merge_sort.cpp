#include "algorithms.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dialsort {

namespace {

void mergeRun(int32_t* src, int32_t* dst,
              std::size_t lo, std::size_t mid, std::size_t hi) {
    std::size_t i = lo;
    std::size_t j = mid;
    for (std::size_t k = lo; k < hi; ++k) {
        if (i >= mid) {
            dst[k] = src[j++];
        } else if (j >= hi) {
            dst[k] = src[i++];
        } else if (src[j] < src[i]) {
            dst[k] = src[j++];
        } else {
            dst[k] = src[i++];
        }
    }
}

}  // namespace

void mergeSort(std::vector<int32_t>& a) {
    const std::size_t n = a.size();
    if (n <= 1) return;

    std::vector<int32_t> tmp(n);
    int32_t* src = a.data();
    int32_t* dst = tmp.data();

    for (std::size_t width = 1; width < n; width *= 2) {
        for (std::size_t lo = 0; lo < n; lo += 2 * width) {
            const std::size_t mid = std::min(lo + width, n);
            const std::size_t hi  = std::min(lo + 2 * width, n);
            mergeRun(src, dst, lo, mid, hi);
        }
        std::swap(src, dst);
    }

    // Make sure the sorted result ends up in `a`.
    if (src != a.data()) {
        std::copy(src, src + n, a.data());
    }
}

}  // namespace dialsort
