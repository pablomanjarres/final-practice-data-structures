#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "distributions.hpp"

namespace dialsort {

enum class AlgoId { Dial, Radix, Merge };

const char* algoName(AlgoId a);
bool parseAlgo(const std::string& s, AlgoId& out);

enum class UniverseMode { EqN, TenthN, TenN, Fixed };

const char* universeModeName(UniverseMode m);
bool parseUniverseMode(const std::string& s, UniverseMode& out);

int32_t computeU(UniverseMode mode, std::size_t n, int32_t fixed);

struct BenchConfig {
    std::vector<AlgoId> algos;
    std::vector<std::size_t> sizes;
    std::vector<Distribution> dists;
    UniverseMode universeMode = UniverseMode::EqN;
    int32_t universeFixed = 0;
    std::uint32_t seed = 42;
    int reps = 3;
    int warmupReps = 1;
};

struct BenchRow {
    AlgoId algo;
    std::size_t n;
    int32_t U;
    Distribution dist;
    int reps;
    std::vector<double> timesMs;
    double meanMs = 0.0;
    double medianMs = 0.0;
    double stdMs = 0.0;
    double throughput = 0.0;
    bool correct = false;
    std::string error;
};

// Run the full configuration matrix. Writes CSV rows to `out`, optionally
// preceded by a header line.
// Also writes per-cell progress lines to `progress` (typically std::cerr) so
// that long benchmark runs aren't silent. Pass nullptr to suppress progress.
void runMatrix(const BenchConfig& cfg,
               std::ostream& out,
               bool emitHeader,
               std::ostream* progress);

}  // namespace dialsort
