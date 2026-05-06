#include "stats.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace dialsort {

double mean(const std::vector<double>& xs) {
    if (xs.empty()) return 0.0;
    double s = 0.0;
    for (double x : xs) s += x;
    return s / static_cast<double>(xs.size());
}

double median(std::vector<double> xs) {
    if (xs.empty()) return 0.0;
    std::sort(xs.begin(), xs.end());
    const std::size_t m = xs.size();
    if (m % 2 == 1) return xs[(m - 1) / 2];
    return (xs[m / 2 - 1] + xs[m / 2]) / 2.0;
}

double stdev(const std::vector<double>& xs) {
    if (xs.size() < 2) return 0.0;
    const double m = mean(xs);
    double s = 0.0;
    for (double x : xs) {
        const double d = x - m;
        s += d * d;
    }
    return std::sqrt(s / static_cast<double>(xs.size() - 1));
}

bool isSortedAscending(const std::vector<int32_t>& a) {
    for (std::size_t i = 1; i < a.size(); ++i) {
        if (a[i] < a[i - 1]) return false;
    }
    return true;
}

bool sameMultiset(std::vector<int32_t> a, std::vector<int32_t> b) {
    if (a.size() != b.size()) return false;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    return a == b;
}

}  // namespace dialsort
