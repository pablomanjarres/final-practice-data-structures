#include "runner.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "algorithms.hpp"
#include "distributions.hpp"
#include "stats.hpp"

namespace dialsort {

const char* algoName(AlgoId a) {
    switch (a) {
        case AlgoId::Dial:  return "dial";
        case AlgoId::Radix: return "radix";
        case AlgoId::Merge: return "merge";
    }
    return "unknown";
}

bool parseAlgo(const std::string& s, AlgoId& out) {
    if (s == "dial")     { out = AlgoId::Dial;  return true; }
    if (s == "radix")    { out = AlgoId::Radix; return true; }
    if (s == "merge")    { out = AlgoId::Merge; return true; }
    return false;
}

const char* universeModeName(UniverseMode m) {
    switch (m) {
        case UniverseMode::EqN:    return "eqN";
        case UniverseMode::TenthN: return "tenthN";
        case UniverseMode::TenN:   return "tenN";
        case UniverseMode::Fixed:  return "fixed";
    }
    return "unknown";
}

bool parseUniverseMode(const std::string& s, UniverseMode& out) {
    if (s == "eqN")    { out = UniverseMode::EqN;    return true; }
    if (s == "tenthN") { out = UniverseMode::TenthN; return true; }
    if (s == "tenN")   { out = UniverseMode::TenN;   return true; }
    if (s == "fixed")  { out = UniverseMode::Fixed;  return true; }
    return false;
}

int32_t computeU(UniverseMode mode, std::size_t n, int32_t fixed) {
    switch (mode) {
        case UniverseMode::EqN:
            return static_cast<int32_t>(std::min<std::size_t>(n, INT32_MAX));
        case UniverseMode::TenthN:
            return std::max(int32_t{1},
                static_cast<int32_t>(std::min<std::size_t>(n / 10, INT32_MAX)));
        case UniverseMode::TenN:
            return static_cast<int32_t>(std::min<std::size_t>(n * 10, INT32_MAX));
        case UniverseMode::Fixed:
            return std::max(int32_t{1}, fixed);
    }
    return static_cast<int32_t>(n);
}

namespace {

double timedRun(AlgoId algo, std::vector<int32_t>& work, int32_t U) {
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();
    switch (algo) {
        case AlgoId::Dial:  dialSort(work, U); break;
        case AlgoId::Radix: radixSortLSD(work); break;
        case AlgoId::Merge: mergeSort(work); break;
    }
    const auto t1 = clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

void writeCsvHeader(std::ostream& out) {
    out << "algo,n,U,distribution,reps,meanMs,medianMs,stdMs,throughput,correct,error,timesMs\n";
    out.flush();
}

void writeTableHeader(std::ostream& out) {
    out << std::left
        << std::setw(7)  << "algo"
        << std::right
        << std::setw(10) << "n"
        << std::setw(10) << "U"
        << "  "
        << std::left
        << std::setw(10) << "dist"
        << std::right
        << std::setw(5)  << "reps"
        << std::setw(11) << "mean(ms)"
        << std::setw(11) << "median(ms)"
        << std::setw(10) << "std(ms)"
        << std::setw(11) << "thr(M/s)"
        << "  ok\n";
    out << std::string(7 + 10 + 10 + 2 + 10 + 5 + 11 + 11 + 10 + 11 + 5, '-') << "\n";
    out.flush();
}

void writeTableRow(std::ostream& out, const BenchRow& r) {
    out << std::left
        << std::setw(7)  << algoName(r.algo)
        << std::right
        << std::setw(10) << r.n
        << std::setw(10) << r.U
        << "  "
        << std::left
        << std::setw(10) << distName(r.dist)
        << std::right
        << std::setw(5)  << r.reps
        << std::setw(11) << std::fixed << std::setprecision(2) << r.meanMs
        << std::setw(11) << std::fixed << std::setprecision(2) << r.medianMs
        << std::setw(10) << std::fixed << std::setprecision(2) << r.stdMs
        << std::setw(11) << std::fixed << std::setprecision(2) << (r.throughput / 1e6)
        << "  " << (r.error.empty() ? (r.correct ? "yes" : "NO ") : "SKIP")
        << "\n";
    out.flush();
}

void writeCsvRow(std::ostream& out, const BenchRow& r) {
    out << algoName(r.algo) << ","
        << r.n << ","
        << r.U << ","
        << distName(r.dist) << ","
        << r.reps << ","
        << r.meanMs << ","
        << r.medianMs << ","
        << r.stdMs << ","
        << r.throughput << ","
        << (r.correct ? "true" : "false") << ",";
    // error field: surround in double quotes; escape internal quotes
    out << "\"";
    for (char c : r.error) {
        if (c == '"') out << "\"\"";
        else out << c;
    }
    out << "\",";
    // timesMs field as quoted bracketed list separated by ;
    out << "\"[";
    for (std::size_t i = 0; i < r.timesMs.size(); ++i) {
        if (i) out << ";";
        out << r.timesMs[i];
    }
    out << "]\"\n";
    out.flush();
}

}  // namespace

void runMatrix(const BenchConfig& cfg,
               std::ostream& out,
               bool emitHeader,
               std::ostream* progress,
               OutputFormat format) {
    const bool table = (format == OutputFormat::Table);
    if (emitHeader) {
        if (table) writeTableHeader(out);
        else       writeCsvHeader(out);
    }
    auto emitRow = [&](const BenchRow& r) {
        if (table) writeTableRow(out, r);
        else       writeCsvRow(out, r);
    };

    const std::size_t totalCells =
        cfg.algos.size() * cfg.sizes.size() * cfg.dists.size();
    std::size_t cellIdx = 0;

    for (std::size_t n : cfg.sizes) {
        const int32_t U = computeU(cfg.universeMode, n, cfg.universeFixed);

        for (Distribution dist : cfg.dists) {
            const auto baseInput = generateInput(dist, n, U, cfg.seed);

            for (AlgoId algo : cfg.algos) {
                BenchRow row;
                row.algo = algo;
                row.n = n;
                row.U = U;
                row.dist = dist;
                row.reps = cfg.reps;

                if (algo == AlgoId::Dial && !dialMemorySafe(n, U)) {
                    const std::size_t bytes = dialEstimatedBytes(n, U);
                    std::ostringstream msg;
                    msg << "DialSort estimate ~" << (bytes / (1024 * 1024))
                        << " MB at U=" << U << ", n=" << n
                        << " exceeds guardrail; skipped.";
                    row.error = msg.str();
                    row.correct = false;
                    emitRow(row);
                    if (progress) {
                        ++cellIdx;
                        *progress << "[" << cellIdx << "/" << totalCells << "] "
                                  << algoName(algo) << " n=" << n
                                  << " U=" << U << " dist=" << distName(dist)
                                  << " — SKIP (memory)\n";
                    }
                    continue;
                }

                for (int w = 0; w < cfg.warmupReps; ++w) {
                    auto work = baseInput;
                    timedRun(algo, work, U);
                }

                std::vector<int32_t> lastResult;
                for (int r = 0; r < cfg.reps; ++r) {
                    auto work = baseInput;
                    const double ms = timedRun(algo, work, U);
                    row.timesMs.push_back(ms);
                    lastResult = std::move(work);
                }

                row.meanMs   = mean(row.timesMs);
                row.medianMs = median(row.timesMs);
                row.stdMs    = stdev(row.timesMs);
                row.throughput = row.meanMs > 0.0
                    ? static_cast<double>(n) / (row.meanMs / 1000.0)
                    : 0.0;
                row.correct = isSortedAscending(lastResult)
                              && sameMultiset(baseInput, lastResult);

                emitRow(row);

                if (progress) {
                    ++cellIdx;
                    *progress << "[" << cellIdx << "/" << totalCells << "] "
                              << algoName(algo) << " n=" << n
                              << " U=" << U << " dist=" << distName(dist)
                              << " mean=" << row.meanMs << "ms"
                              << " thr=" << (row.throughput / 1e6) << "M/s"
                              << " " << (row.correct ? "OK" : "FAIL") << "\n";
                }
            }
        }
    }
}

}  // namespace dialsort
