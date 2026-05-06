# C++ implementation — algorithms & benchmark harness

This directory holds the **canonical C++ implementations** of the three sorting
algorithms and the benchmark CLI, per the assignment rubric ("Code in C++").

The React app in `../src/` is the **simulation** (visualization) — same
algorithms, ported to TypeScript, used solely to power the in-browser
step-by-step animation.

## Build

```bash
cd cpp
make           # produces ./bench (C++17, -O3, -Wall -Wextra -Wpedantic)
```

Requires `g++` or `clang++` with C++17 support. On macOS install Xcode Command
Line Tools (`xcode-select --install`) if needed.

## Run

```bash
./bench --quick          # ≤1M, uniform, 3 reps  (~3 s on a modern laptop)
./bench --full           # ≤10M, uniform, 3 reps (~30 s)
./bench --memwall        # U = 10·n; demonstrates DialSort memory wall

./bench --help           # full flag reference

# Example: single cell to a CSV file
./bench --algo dial,radix,merge \
        --sizes 1000000 \
        --dists uniform,sorted,reverse,gaussian,sparseDup \
        --reps 5 \
        -o results.csv
```

CSV header:

```
algo,n,U,distribution,reps,meanMs,medianMs,stdMs,throughput,correct,error,timesMs
```

The format matches the React app's CSV export so they can be merged into
`../REPORT.md`.

Per-cell progress is written to `stderr`, so the CSV (`stdout`) stays clean
when piped.

## Files

```
cpp/
├── Makefile
├── include/
│   ├── algorithms.hpp      # dialSort / radixSortLSD / mergeSort + memory guard
│   ├── distributions.hpp   # Distribution enum + generateInput
│   ├── stats.hpp           # mean / median / stdev / isSorted / sameMultiset
│   └── runner.hpp          # AlgoId, UniverseMode, BenchConfig, runMatrix
└── src/
    ├── dial_sort.cpp       # bucket sort, chained vectors, O(n + U)
    ├── radix_sort.cpp      # LSD base-256, 4 passes, O(d(n + k))
    ├── merge_sort.cpp      # bottom-up iterative, O(n log n)
    ├── distributions.cpp   # mt19937 + 5 distributions
    ├── stats.cpp
    ├── runner.cpp          # benchmark matrix runner + CSV emitter
    └── main.cpp            # CLI
```

## Memory guardrail (DialSort)

The implementation in `dial_sort.cpp` allocates `U` empty `std::vector<int32_t>`
buckets up front; each is ~32 B on a 64-bit toolchain. At U ≈ 10⁸ this is
~3 GB just for the bucket headers — DOS-style. The runner therefore checks
`dialMemorySafe(n, U)` before each call and emits a row with
`error="… exceeds guardrail; skipped."` (and `correct=false`) when it would
exceed the limits:

- **Hard limit on U**: `5·10⁷` buckets.
- **Hard limit on bytes**: `500 MiB` estimated.

These limits are tuned to comfortably fit on a typical 16 GB laptop while
still leaving the `--memwall` preset (U = 10·n, n ≤ 10⁷) to cleanly trip them
for didactic effect during the live defense.

## Live defense quick reference

```bash
make                 # 7 .cpp files, builds in ~5 s
./bench --quick      # demo: ≤1M, uniform, 3 reps
./bench --full       # demo: ≤10M; shows asymptotic separation
./bench --memwall    # demo: DialSort skipped at U=10·n at large n
```
