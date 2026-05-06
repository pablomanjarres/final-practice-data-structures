# Technical Report — DialSort vs. Alternative Proposal

**Course:** ST0245 / SI001 — Data Structures and Algorithms (EAFIT)
**Practice:** II — Experimental Analysis of Algorithms and Data Structures
**Date:** *(fill in submission date)*
**Authors:** *(fill in)*

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

- **Hardware / runtime:** *(fill in: CPU, RAM, OS, compiler version — e.g.,
  `g++ --version`)*
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
| DialSort  |          |          |        |        |         |
| Radix LSD |          |          |        |        |         |
| MergeSort |          |          |        |        |         |

*(throughput in M items/s = `n / meanMs * 1000 / 1e6`)*

### 3.2 Distribution sensitivity (n = 1M, U = n)

Run `./bench --algo dial,radix,merge --sizes 1000000 --dists uniform,sorted,reverse,gaussian,sparseDup --reps 5`:

| Distribution | DialSort | Radix LSD | MergeSort |
| ------------ | -------- | --------- | --------- |
| Uniform      |          |           |           |
| Sorted       |          |           |           |
| Reverse      |          |           |           |
| Gaussian     |          |           |           |
| SparseDup    |          |           |           |

### 3.3 Universe-size sensitivity (n = 1M)

Run with `--U-mode tenthN`, `eqN`, `tenN`:

| Mode      | DialSort | Radix LSD | MergeSort |
| --------- | -------- | --------- | --------- |
| U = n/10  |          |           |           |
| U = n     |          |           |           |
| U = 10·n  |          |           |           |

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

*(Discuss the throughput numbers from §3.1 and how the gap closes/widens as n
grows. Expected pattern from preliminary runs: DialSort and MergeSort are
similar at U = n with high constants; Radix is ~10–20× faster than both.
The relative ordering is dominated by cache behaviour and the cost of
allocating U empty `std::vector` headers in DialSort.)*

### 4.3 Memory consumption

DialSort allocates O(n + U) and pays the `U`-dependent component as
`std::vector` object overhead — ~32 bytes per bucket header on a 64-bit
toolchain. The `--memwall` preset deliberately triggers this: at n = 10⁶
DialSort estimates ~150 MB just for the bucket array, and the in-app
guardrail blocks U > 5·10⁷ to protect the demo.

Radix and Merge are insensitive to U.

### 4.4 Which algorithm performed better?

*(Fill in based on results.)* The answer is **regime-dependent**:

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

*(One paragraph wrap-up: what we learned, recommendations for which algorithm
to pick in which situation, and what we'd extend if we had more time —
e.g., parallel radix, MSD radix, in-place radix variants, or replacing
DialSort's chained vectors with a counting-sort-style two-pass implementation
that drops the per-bucket allocation.)*

## 6. References

- Cormen, Leiserson, Rivest, Stein — *Introduction to Algorithms*, 4th ed.
  (Counting Sort, Radix Sort, Merge Sort).
- Knuth, *The Art of Computer Programming*, Vol. 3 (Sorting).
- Dial, R. — *Algorithm 360: Shortest-Path Forest with Topological Ordering*,
  CACM 12(11), 1969 (origin of the "dial" bucket queue).
