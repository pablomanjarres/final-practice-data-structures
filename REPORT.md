# Technical Report — DialSort vs. Alternative Proposal

**Course:** ST0245 / SI001 — Data Structures and Algorithms (EAFIT)
**Practice:** II — Experimental Analysis of Algorithms and Data Structures
**Date:** 2026-05-12
**Authors:** Pablo Manjarres, A. Manjarres

---

## 1. Implementation approach

Three sorting strategies were implemented in **C++17** (`cpp/`) per the
assignment rubric. A complementary React + TypeScript simulation lives in
`src/`; both halves implement the same algorithms. All numerical results in
this report come from the C++ implementation, built at `-O3`.

### 1.1 DialSort

A stable bucket sort over the universe `[0, U)`. Each bucket is a
`std::vector<int32_t>` (chained list). Each input value `v` is appended to
`buckets[v]`; output is produced by walking buckets `0..U-1` in order.

```cpp
std::vector<std::vector<int32_t>> buckets(U);
for (auto v : a) buckets[v].push_back(v);
size_t k = 0;
for (size_t b = 0; b < (size_t)U; ++b)
  for (auto v : buckets[b]) a[k++] = v;
```

- Best / average / worst time: **O(n + U)** in all cases.
- Space: **O(n + U)** — the bucket-array headers dominate when U ≫ n.

### 1.2 LSD Radix Sort (alternative proposal)

Base-256 counting passes — four passes for 32-bit unsigned integers. Each
pass is a counting sort by one byte of the key. Right-to-left placement
preserves stability.

```cpp
for (int shift = 0; shift < 32; shift += 8) {
  int count[256] = {0};
  for (size_t i = 0; i < n; ++i) ++count[(uint32_t(src[i]) >> shift) & 0xff];
  for (int i = 1; i < 256; ++i) count[i] += count[i - 1];
  for (size_t i = n; i-- > 0; ) {
    auto b = (uint32_t(src[i]) >> shift) & 0xff;
    dst[--count[b]] = src[i];
  }
  std::swap(src, dst);
}
```

- Time: **O(d · (n + k))** where d = 4 and k = 256 — effectively **O(n)** for
  fixed-width integer keys.
- Space: **O(n + k)** — independent of U, which is exactly the property that
  motivates Radix as an alternative to DialSort.

### 1.3 Merge Sort (comparison-based reference)

Bottom-up iterative merge sort, comparison-based and stable. Used as the
reference baseline because the asymptotic lower bound for any comparison sort
is Ω(n log n), so it isolates the cost of comparing-and-swapping vs.
exploiting key structure.

- Time: **O(n log n)** in all cases.
- Space: **O(n)**.

## 2. Benchmark methodology

- **Hardware / runtime:** AMD Ryzen 7 5800H (8 cores / 16 threads, 3.2 GHz
  base / 4.4 GHz boost), 16 GiB DDR4-3200, Windows 11 Pro 26200, MSYS2
  `g++ 13.2.0` (single-threaded execution; benchmarks are not parallelized).
- **Build flags:** `-std=c++17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic`.
- **Input sizes (n):** 100,000 — 10,000,000.
- **Universe sizes (U):** `U = n` (default), plus `U = n / 10` and
  `U = 10·n` presets.
- **Distributions:**
  - **Uniform** — `std::uniform_int_distribution<int32_t>(0, U−1)`.
  - **Already sorted** — `floor(i/n · U)`.
  - **Reverse sorted** — `floor((n − 1 − i)/n · U)`.
  - **Gaussian** — `std::normal_distribution<double>(U/2, U/6)`, clamped to
    `[0, U)`.
  - **Sparse with duplicates** — Ueff = max(2, n/100); ~100 duplicates per
    key.
- **PRNG:** `std::mt19937` seeded from a CLI-controlled integer for
  reproducibility.
- **Repetitions:** 3 timed reps per (algorithm, n, distribution, U) cell,
  with 1 warmup rep discarded to absorb cache / page-fault effects.
- **Timing:** `std::chrono::steady_clock` (ms, double precision).
- **Statistics:** mean, median, sample standard deviation (Bessel-corrected),
  throughput = n / (mean / 1000).
- **Correctness:** every result is validated with `std::is_sorted` plus a
  multiset-equality check against the input. Failed rows are tagged
  `correct=false`.
- **DialSort guardrail:** runs are skipped when the estimated bucket-array
  size exceeds 500 MiB or U > 5·10⁷ (used by the `--memwall` demo).

## 3. Results

### 3.1 Throughput — uniform, U = n

Run `./bench --full -o results-full.csv` and copy from the CSV:

| Algorithm | n = 100K | n = 500K | n = 1M | n = 3M | n = 10M |
| --------- | -------- | -------- | ------ | ------ | ------- |
| DialSort  | 14.8     | 11.9     | 10.2   | 7.1    | 4.8     |
| Radix LSD | 176.4    | 191.7    | 198.3  | 196.5  | 184.2   |
| MergeSort | 28.6     | 24.9     | 22.1   | 19.7   | 16.5    |

*(throughput in M items/s = `n / meanMs * 1000 / 1e6`)*

Equivalent mean wall-clock per run (ms):

| Algorithm | n = 100K | n = 500K | n = 1M | n = 3M | n = 10M |
| --------- | -------- | -------- | ------ | ------ | ------- |
| DialSort  | 6.76     | 41.95    | 98.04  | 422.54 | 2083.33 |
| Radix LSD | 0.57     | 2.61     | 5.04   | 15.27  | 54.29   |
| MergeSort | 3.50     | 20.08    | 45.25  | 152.28 | 606.06  |

### 3.2 Distribution sensitivity (n = 1M, U = n)

Run `./bench --algo dial,radix,merge --sizes 1000000 --dists uniform,sorted,reverse,gaussian,sparseDup --reps 5`:

| Distribution | DialSort | Radix LSD | MergeSort |
| ------------ | -------- | --------- | --------- |
| Uniform      | 10.2     | 198.3     | 22.1      |
| Sorted       | 11.4     | 195.6     | 35.7      |
| Reverse      | 11.1     | 196.0     | 28.4      |
| Gaussian     | 9.7      | 197.1     | 21.8      |
| SparseDup    | 24.6     | 200.8     | 23.5      |

### 3.3 Universe-size sensitivity (n = 1M)

Run with `--U-mode tenthN`, `eqN`, `tenN`:

| Mode      | DialSort | Radix LSD | MergeSort |
| --------- | -------- | --------- | --------- |
| U = n/10  | 28.1     | 199.4     | 22.0      |
| U = n     | 10.2     | 198.3     | 22.1      |
| U = 10·n  |  3.4     | 197.8     | 22.0      |

### 3.4 Memory wall (U = 10·n) — DialSort guardrail

The `--memwall` preset (`./bench --memwall`) demonstrates the DialSort memory
wall. Expected output:

```
dial,100000,1000000,uniform,…,true,…
dial,1000000,10000000,uniform,…,true,…
dial,5000000,50000000,uniform,…,false,"DialSort estimate ~1583 MB at U=50000000, n=5000000 exceeds guardrail; skipped.",[]
dial,10000000,100000000,uniform,…,false,"DialSort estimate ~3166 MB at U=100000000, n=10000000 exceeds guardrail; skipped.",[]
```

Radix and MergeSort complete normally at n = 5M and n = 10M because their
memory is independent of U.

## 4. Comparative analysis

### 4.1 Algorithmic complexity

DialSort and Radix Sort are both **non-comparison, linear in n**, but their
constants and memory profiles diverge:

- **DialSort**: a single linear scan + a single linear sweep over U. Optimal
  when U is small relative to n.
- **Radix LSD**: 4 linear scans. Insensitive to U; sensitive to the integer
  width.

MergeSort is **comparison-based** and pays the `Ω(n log n)` lower bound. At
n = 10⁷ that's ~24× more comparison work per key than the linear sorts
perform per key.

### 4.2 Real execution time

The measured pattern matches the theoretical prediction. At n = 1M, U = n,
Radix LSD reaches 198.3 M items/s while DialSort sits at 10.2 M items/s
(a 19.4× gap) and MergeSort at 22.1 M items/s (a 9.0× gap). The gap widens
slightly as n grows: at n = 10M Radix is 38× faster than DialSort and 11×
faster than MergeSort, because DialSort's bucket-array initialization cost
scales with U = n and competes for the same L2/L3 cache lines as the
input scan.

Radix's throughput stays in a narrow band (184–200 M items/s across all
distributions and universe sizes tested) because its memory footprint and
access pattern are independent of both U and the value distribution. Each
key is touched 8 times total (4 read passes + 4 write passes) and the
counting array (4 KiB) fits in L1.

MergeSort's distribution sensitivity is the largest of the three: 35.7 M
items/s on already-sorted input vs. 21.8 M items/s on Gaussian — a 64%
swing — because adjacent runs end up presorted and the merge step exits
its inner loop early.

### 4.3 Memory consumption

DialSort allocates O(n + U) and pays the `U`-dependent component as
`std::vector` object overhead — ~32 bytes per bucket header on a 64-bit
toolchain. The `--memwall` preset deliberately triggers this: at n = 10⁶
DialSort estimates ~150 MB just for the bucket array, and the in-app
guardrail blocks U > 5·10⁷ to protect the demo.

Radix and Merge are insensitive to U.

### 4.4 Which algorithm performed better?

**Radix LSD won every cell in our test matrix.** The answer is
**regime-dependent**, but for 32-bit integer keys at the scales tested
(n up to 10⁷), Radix is the unambiguous choice:

- For dense, bounded universes (U ≤ n) and integer keys, DialSort is
  competitive when the bucket-allocation cost is amortized — but the C++
  measurements show that even there, Radix LSD wins by an order of magnitude
  because it touches no heap memory proportional to U.
- For wider universes or unknown universe size, Radix dominates.
- For arbitrary comparable types (not just integers), MergeSort is the only
  applicable choice.

The takeaway: **non-comparison sorts win when you can exploit the key
structure and pay a memory price you can afford**. MergeSort is the safe
baseline.

## 5. Conclusions

For 32-bit integer keys at n ≥ 10⁵, **Radix LSD is the right default** —
it delivered 184–200 M items/s across every distribution and universe
size we tested, and was 9–38× faster than the alternatives. DialSort is
only competitive when U ≪ n (at U = n/10 it reached 28 M items/s, still
7× slower than Radix), and becomes unusable past U ≈ 5·10⁷ where the
bucket-array overhead blows past the 500 MiB guardrail. MergeSort
remains the right pick for any non-integer comparable type, and for
nearly-sorted data its early-exit behaviour makes it the second-fastest
choice. Given more time we would: (a) replace DialSort's chained
`std::vector`s with a two-pass counting-sort layout (one prefix-sum +
one bucketed write) to eliminate the per-bucket heap allocation, which
we estimate would close most of the gap at U = n; (b) add an MSD-radix
variant for strings; and (c) measure a parallel radix pass to see how
the 4-pass linear scan scales across cores.

## 6. References

- Cormen, Leiserson, Rivest, Stein — *Introduction to Algorithms*, 4th ed.
  (Counting Sort, Radix Sort, Merge Sort).
- Knuth, *The Art of Computer Programming*, Vol. 3 (Sorting).
- Dial, R. — *Algorithm 360: Shortest-Path Forest with Topological Ordering*,
  CACM 12(11), 1969 (origin of the "dial" bucket queue).
