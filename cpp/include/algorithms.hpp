#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dialsort {

// In-place stable bucket sort over universe [0, U).
// Time:  O(n + U) best/avg/worst.
// Space: O(n + U).
// Pre:   every element of `a` is in [0, U).
void dialSort(std::vector<int32_t>& a, int32_t U);

// In-place LSD radix sort, base 256, 4 passes for 32-bit non-negative ints.
// Time:  O(d * (n + k)) with d = 4, k = 256.
// Space: O(n + k).
void radixSortLSD(std::vector<int32_t>& a);

// In-place bottom-up iterative merge sort. Comparison-based, stable.
// Time:  O(n log n) all cases.
// Space: O(n).
void mergeSort(std::vector<int32_t>& a);

// Memory-safety guard for DialSort. Returns true iff running DialSort
// at (n, U) is expected to fit comfortably in memory.
bool dialMemorySafe(std::size_t n, int32_t U);

// Estimated bytes DialSort will allocate at (n, U). Used for the guard.
std::size_t dialEstimatedBytes(std::size_t n, int32_t U);

}  // namespace dialsort
