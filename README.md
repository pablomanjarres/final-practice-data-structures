# Practice II — DialSort vs. Alternative Proposal

**Course:** ST0245 / SI001 — Data Structures and Algorithms
**Institution:** EAFIT University, School of Applied Sciences and Engineering
**Lecturer:** Alexander Narváez Berrío
**Term:** April 2026 · Value: 20 % of final grade

## Team members

- *(fill in your names + IDs before submission)*

## Architecture

The assignment rubric requires "Code in C++" with a free-form simulation. The
project is therefore split in two:

| Part            | Language             | Location  | Purpose                                                                  |
| --------------- | -------------------- | --------- | ------------------------------------------------------------------------ |
| **Algorithms + benchmark** | **C++17**            | `cpp/`    | Canonical implementations of the three sorts and the benchmark CLI.      |
| **Simulation (visualizer)** | TypeScript + React | `src/`    | Step-by-step in-browser animation. Run with `npm run dev`.               |

Both halves implement the same three algorithms. The TypeScript copies in
`src/algorithms/` are not "duplicates of convenience" — they exist because
the visualizer's animation requires step-yielding generators that have to run
in the browser.

## The three sorting strategies

| Algorithm                         | Best       | Average    | Worst      | Space    | Notes                                  |
| --------------------------------- | ---------- | ---------- | ---------- | -------- | -------------------------------------- |
| **DialSort**                      | O(n + U)   | O(n + U)   | O(n + U)   | O(n + U) | Bucket sort, chained lists per bucket. |
| **LSD Radix Sort** (alternative)  | O(d(n+k))  | O(d(n+k))  | O(d(n+k))  | O(n + k) | Base 256, 4 passes for 32-bit ints.    |
| **Merge Sort** (comparison ref.)  | O(n log n) | O(n log n) | O(n log n) | O(n)     | Bottom-up iterative.                   |

`d = 4`, `k = 256` for 32-bit integer keys.

## Running the C++ benchmark

```bash
cd cpp
make                    # builds ./bench at -O3
./bench --quick         # n ≤ 1M, uniform, 3 reps         (~3 s)
./bench --full          # n ≤ 10M, uniform, 3 reps        (~30 s)
./bench --memwall       # U = 10·n; demonstrates the wall (~30 s)
./bench --help          # full flag reference
```

CSV columns: `algo, n, U, distribution, reps, meanMs, medianMs, stdMs, throughput, correct, error, timesMs`.

Per-cell progress goes to `stderr`, so CSV piping stays clean.

See [cpp/README.md](cpp/README.md) for full details.

## Running the React simulation

```bash
npm install
npm run dev             # serves at http://localhost:5173
```

A single visualizer page: pick an algorithm (DialSort / Radix / Merge), input
size (n ≤ 128), distribution (uniform / sorted / reverse), seed, and speed.
Play / pause / step / back-step controls drive the canvas animation.

The simulation is deliberately minimal — **the canonical algorithms and the
real benchmark live in `cpp/`.** The TS step-yielding generators in
`src/algorithms.ts` exist only to power the in-browser animation.

Other npm scripts:

```bash
npm run build           # production build → dist/
npm run preview         # preview the production build
npm run typecheck       # TypeScript-only check, no emit
```

## Live-demo flow (in-class defense)

1. **C++ benchmark** — `cd cpp && make && ./bench --quick`. Show the CSV
   scrolling and per-cell `[i/12] mean=… thr=… OK` progress on stderr.
2. **C++ memory wall** — `./bench --memwall`. Show `dial,5000000,…,SKIP` rows
   when U exceeds the 5·10⁷ guardrail, while Radix and Merge complete.
3. **React visualizer** — `cd .. && npm run dev`, open
   `http://localhost:5173`. Pick each algorithm at size 32, hit Play. Narrate
   the internal behavior (buckets filling/draining for DialSort; per-pass
   digit counts for Radix; merge ranges for Merge).
4. **Paste numbers into the report** — pipe `./bench --full` to a CSV and
   copy the throughput values into [REPORT.md](REPORT.md) §3.

## Project structure

```
final-practice-data-structures/
├── cpp/                        # C++ implementation (assignment-mandated)
│   ├── Makefile
│   ├── include/{algorithms,distributions,stats,runner}.hpp
│   ├── src/{dial_sort,radix_sort,merge_sort,distributions,stats,runner,main}.cpp
│   └── README.md
├── src/                        # React simulation (6 files, intentionally minimal)
│   ├── algorithms.ts           # 3 step-yielding sort generators + input gen
│   ├── Visualizer.tsx          # canvas + controls in one component
│   ├── App.tsx                 # trivial wrapper
│   ├── main.tsx                # Vite entry
│   ├── styles.css
│   └── vite-env.d.ts
├── resources/                  # assignment PDF
├── REPORT.md                   # technical report
├── README.md                   # this file
├── package.json, vite.config.ts, tsconfig.json, index.html
```

## Correctness

Every C++ benchmark row reports `correct=true|false` (sorted and
multiset-equal to input). The `make verify` target runs a small sweep across
all five distributions to confirm correctness in seconds.

## Notes on "DialSort"

"DialSort" is not a fully standardized textbook name. We implement it as a
stable bucket sort with chained lists per bucket — the most common
interpretation when the universe size U is known up front, and the one that
matches the algorithm's memory behavior described by the assignment. If your
instructor expects the priority-queue-style Dial structure (the bucket-queue
shortest-path datastructure named after Dial 1969), the implementation in
`cpp/src/dial_sort.cpp` is small enough to swap.

## Known limitations

- **C++ memory guardrail**: DialSort runs are skipped when the estimated
  bucket-array size exceeds 500 MiB or U > 5·10⁷. This is intentional and
  forms the `--memwall` defense demo.
