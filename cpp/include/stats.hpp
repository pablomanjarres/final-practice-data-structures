#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dialsort {

double mean(const std::vector<double>& xs);
double median(std::vector<double> xs);  // copies on purpose
double stdev(const std::vector<double>& xs);

bool isSortedAscending(const std::vector<int32_t>& a);

// Equality up to permutation. O(n log n) — sorts copies and compares.
bool sameMultiset(std::vector<int32_t> a, std::vector<int32_t> b);

}  // namespace dialsort
