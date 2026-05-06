// dialsort-bench — DialSort vs. Alternative comparative benchmark
// EAFIT ST0245 / SI001 — Practice II — April 2026
//
// Build:  make
// Usage:  see --help

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "distributions.hpp"
#include "runner.hpp"

using namespace dialsort;

namespace {

void printHelp() {
    std::cout <<
        "dialsort-bench — DialSort vs. Alternative comparative benchmark\n"
        "\n"
        "Usage:\n"
        "  bench [PRESET] [OPTIONS]\n"
        "\n"
        "Presets (override individual flags):\n"
        "  --quick        n up to 1M, uniform, 3 reps (~5 s)\n"
        "  --full         n up to 10M, uniform, 3 reps (~30 s)\n"
        "  --memwall      U = 10*n; demonstrates DialSort memory wall\n"
        "\n"
        "Options:\n"
        "  --algo LIST          comma list from {dial,radix,merge}; default all\n"
        "  --sizes LIST         comma list of integers (n)\n"
        "  --dists LIST         comma list from\n"
        "                       {uniform,sorted,reverse,gaussian,sparseDup}\n"
        "  --U-mode MODE        eqN | tenthN | tenN | fixed   (default eqN)\n"
        "  --U-fixed N          when --U-mode fixed\n"
        "  --reps N             timed reps per cell (default 3)\n"
        "  --warmup N           warmup reps (default 1)\n"
        "  --seed N             RNG seed (default 42)\n"
        "  -o, --out FILE       write CSV to FILE (default: stdout)\n"
        "  --no-header          omit CSV header line\n"
        "  --no-progress        omit per-cell progress on stderr\n"
        "  -h, --help           this help\n"
        "\n"
        "Output: CSV — algo,n,U,distribution,reps,meanMs,medianMs,stdMs,\n"
        "              throughput,correct,error,timesMs\n";
}

bool nextArg(int& i, int argc, char** argv, std::string& out) {
    if (i + 1 >= argc) return false;
    out = argv[++i];
    return true;
}

std::vector<std::string> splitCsv(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ',') { out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

bool parseSizes(const std::string& csv, std::vector<std::size_t>& out) {
    out.clear();
    for (const auto& part : splitCsv(csv)) {
        try {
            out.push_back(static_cast<std::size_t>(std::stoull(part)));
        } catch (...) {
            std::cerr << "Bad size: " << part << "\n";
            return false;
        }
    }
    return !out.empty();
}

bool parseAlgos(const std::string& csv, std::vector<AlgoId>& out) {
    out.clear();
    if (csv == "all") {
        out = {AlgoId::Dial, AlgoId::Radix, AlgoId::Merge};
        return true;
    }
    for (const auto& part : splitCsv(csv)) {
        AlgoId a;
        if (!parseAlgo(part, a)) {
            std::cerr << "Bad algo: " << part << "\n";
            return false;
        }
        out.push_back(a);
    }
    return !out.empty();
}

bool parseDists(const std::string& csv, std::vector<Distribution>& out) {
    out.clear();
    if (csv == "all") {
        out = {Distribution::Uniform, Distribution::Sorted,
               Distribution::Reverse, Distribution::Gaussian,
               Distribution::SparseDup};
        return true;
    }
    for (const auto& part : splitCsv(csv)) {
        Distribution d;
        if (!parseDistribution(part, d)) {
            std::cerr << "Bad distribution: " << part << "\n";
            return false;
        }
        out.push_back(d);
    }
    return !out.empty();
}

void applyPreset(const std::string& name, BenchConfig& cfg) {
    cfg.algos = {AlgoId::Dial, AlgoId::Radix, AlgoId::Merge};
    cfg.dists = {Distribution::Uniform};
    cfg.universeMode = UniverseMode::EqN;
    cfg.reps = 3;
    cfg.warmupReps = 1;

    if (name == "quick") {
        cfg.sizes = {100'000, 250'000, 500'000, 1'000'000};
    } else if (name == "full") {
        cfg.sizes = {100'000, 500'000, 1'000'000, 3'000'000, 10'000'000};
    } else if (name == "memwall") {
        cfg.sizes = {100'000, 1'000'000, 5'000'000, 10'000'000};
        cfg.universeMode = UniverseMode::TenN;
    }
}

}  // namespace

int main(int argc, char** argv) {
    BenchConfig cfg;
    cfg.algos = {AlgoId::Dial, AlgoId::Radix, AlgoId::Merge};
    cfg.sizes = {100'000, 250'000, 500'000, 1'000'000};
    cfg.dists = {Distribution::Uniform};
    cfg.universeMode = UniverseMode::EqN;
    cfg.universeFixed = 0;
    cfg.seed = 42;
    cfg.reps = 3;
    cfg.warmupReps = 1;

    std::string outPath;
    bool emitHeader = true;
    bool emitProgress = true;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        std::string v;
        if (a == "-h" || a == "--help") {
            printHelp();
            return 0;
        } else if (a == "--quick" || a == "--full" || a == "--memwall") {
            applyPreset(a.substr(2), cfg);
        } else if (a == "--algo") {
            if (!nextArg(i, argc, argv, v) || !parseAlgos(v, cfg.algos)) return 2;
        } else if (a == "--sizes") {
            if (!nextArg(i, argc, argv, v) || !parseSizes(v, cfg.sizes)) return 2;
        } else if (a == "--dists") {
            if (!nextArg(i, argc, argv, v) || !parseDists(v, cfg.dists)) return 2;
        } else if (a == "--U-mode") {
            if (!nextArg(i, argc, argv, v) ||
                !parseUniverseMode(v, cfg.universeMode)) return 2;
        } else if (a == "--U-fixed") {
            if (!nextArg(i, argc, argv, v)) return 2;
            cfg.universeFixed = static_cast<int32_t>(std::stol(v));
        } else if (a == "--reps") {
            if (!nextArg(i, argc, argv, v)) return 2;
            cfg.reps = std::max(1, std::atoi(v.c_str()));
        } else if (a == "--warmup") {
            if (!nextArg(i, argc, argv, v)) return 2;
            cfg.warmupReps = std::max(0, std::atoi(v.c_str()));
        } else if (a == "--seed") {
            if (!nextArg(i, argc, argv, v)) return 2;
            cfg.seed = static_cast<std::uint32_t>(std::stoul(v));
        } else if (a == "-o" || a == "--out") {
            if (!nextArg(i, argc, argv, outPath)) return 2;
        } else if (a == "--no-header") {
            emitHeader = false;
        } else if (a == "--no-progress") {
            emitProgress = false;
        } else {
            std::cerr << "Unknown argument: " << a << "\n"
                      << "Run with --help for usage.\n";
            return 2;
        }
    }

    std::ofstream fileOut;
    std::ostream* out = &std::cout;
    if (!outPath.empty()) {
        fileOut.open(outPath);
        if (!fileOut.is_open()) {
            std::cerr << "Cannot open output file: " << outPath << "\n";
            return 1;
        }
        out = &fileOut;
    }

    runMatrix(cfg, *out, emitHeader, emitProgress ? &std::cerr : nullptr);

    return 0;
}
