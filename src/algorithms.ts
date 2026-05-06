// Step-yielding ports of the three sorts for the in-browser visualizer.
// The canonical implementations live in cpp/.

export type AlgoId = 'dial' | 'radix' | 'merge';

export const ALGO_LABELS: Record<AlgoId, string> = {
  dial: 'DialSort',
  radix: 'Radix Sort (LSD)',
  merge: 'Merge Sort',
};

export const ALGO_BIG_O: Record<AlgoId, string> = {
  dial: 'O(n + U)',
  radix: 'O(d·(n + k))',
  merge: 'O(n log n)',
};

export type Distribution = 'uniform' | 'sorted' | 'reverse';

export const DISTRIBUTION_LABELS: Record<Distribution, string> = {
  uniform: 'Uniform',
  sorted: 'Already sorted',
  reverse: 'Reverse sorted',
};

export type SortStep =
  | { kind: 'read'; i: number }
  | { kind: 'write'; i: number; v: number }
  | { kind: 'bucketPush'; bucket: number; v: number; sourceIdx: number }
  | { kind: 'bucketEmit'; bucket: number; v: number; destIdx: number }
  | { kind: 'radixCount'; digit: number; bucket: number; v: number; sourceIdx: number }
  | { kind: 'radixEmit'; digit: number; bucket: number; v: number; destIdx: number }
  | { kind: 'radixPass'; digit: number; snapshot: Int32Array }
  | { kind: 'compare'; i: number; j: number }
  | { kind: 'mergeRange'; lo: number; mid: number; hi: number }
  | { kind: 'done'; result: Int32Array };

// mulberry32 — small seeded PRNG, good enough for demo inputs
function makeRng(seed: number): () => number {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) | 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export function generateInput(
  dist: Distribution,
  n: number,
  U: number,
  seed: number
): Int32Array {
  const out = new Int32Array(n);
  if (dist === 'sorted') {
    for (let i = 0; i < n; i++) out[i] = Math.floor((i / n) * U);
  } else if (dist === 'reverse') {
    for (let i = 0; i < n; i++) out[i] = Math.floor(((n - 1 - i) / n) * U);
  } else {
    const rng = makeRng(seed);
    for (let i = 0; i < n; i++) out[i] = Math.floor(rng() * U);
  }
  return out;
}

export function* dialSortSteps(
  input: Int32Array,
  U: number
): Generator<SortStep, Int32Array, void> {
  const n = input.length;
  const buckets: number[][] = new Array(U);
  for (let b = 0; b < U; b++) buckets[b] = [];

  for (let i = 0; i < n; i++) {
    yield { kind: 'read', i };
    const v = input[i];
    buckets[v].push(v);
    yield { kind: 'bucketPush', bucket: v, v, sourceIdx: i };
  }

  const out = new Int32Array(n);
  let k = 0;
  for (let b = 0; b < U; b++) {
    const bucket = buckets[b];
    for (let i = 0; i < bucket.length; i++) {
      out[k] = bucket[i];
      yield { kind: 'bucketEmit', bucket: b, v: bucket[i], destIdx: k };
      k++;
    }
  }

  yield { kind: 'done', result: out };
  return out;
}

export function* radixSortLSDSteps(
  input: Int32Array
): Generator<SortStep, Int32Array, void> {
  const n = input.length;
  if (n === 0) {
    const empty = new Int32Array(0);
    yield { kind: 'done', result: empty };
    return empty;
  }

  let src = new Int32Array(input);
  let dst = new Int32Array(n);
  const k = 256;

  for (let shift = 0; shift < 32; shift += 8) {
    const digit = shift / 8;
    const count = new Int32Array(k);

    for (let i = 0; i < n; i++) {
      yield { kind: 'read', i };
      const b = (src[i] >>> shift) & 0xff;
      count[b]++;
      yield { kind: 'radixCount', digit, bucket: b, v: src[i], sourceIdx: i };
    }
    for (let i = 1; i < k; i++) count[i] += count[i - 1];

    for (let i = n - 1; i >= 0; i--) {
      const b = (src[i] >>> shift) & 0xff;
      const destIdx = --count[b];
      dst[destIdx] = src[i];
      yield { kind: 'radixEmit', digit, bucket: b, v: src[i], destIdx };
    }

    yield { kind: 'radixPass', digit, snapshot: new Int32Array(dst) };
    const tmp = src;
    src = dst;
    dst = tmp;
  }

  yield { kind: 'done', result: src };
  return src;
}

export function* mergeSortSteps(
  input: Int32Array
): Generator<SortStep, Int32Array, void> {
  const n = input.length;
  const a = new Int32Array(input);
  const aux = new Int32Array(n);
  if (n > 1) yield* sortRange(a, aux, 0, n - 1);
  yield { kind: 'done', result: a };
  return a;
}

function* sortRange(
  a: Int32Array,
  aux: Int32Array,
  lo: number,
  hi: number
): Generator<SortStep, void, void> {
  if (lo >= hi) return;
  const mid = (lo + hi) >>> 1;
  yield* sortRange(a, aux, lo, mid);
  yield* sortRange(a, aux, mid + 1, hi);
  yield* mergeRange(a, aux, lo, mid, hi);
}

function* mergeRange(
  a: Int32Array,
  aux: Int32Array,
  lo: number,
  mid: number,
  hi: number
): Generator<SortStep, void, void> {
  yield { kind: 'mergeRange', lo, mid, hi };
  for (let k = lo; k <= hi; k++) aux[k] = a[k];

  let i = lo;
  let j = mid + 1;
  for (let k = lo; k <= hi; k++) {
    if (i > mid) {
      a[k] = aux[j++];
      yield { kind: 'write', i: k, v: a[k] };
    } else if (j > hi) {
      a[k] = aux[i++];
      yield { kind: 'write', i: k, v: a[k] };
    } else {
      yield { kind: 'compare', i, j };
      if (aux[j] < aux[i]) {
        a[k] = aux[j++];
      } else {
        a[k] = aux[i++];
      }
      yield { kind: 'write', i: k, v: a[k] };
    }
  }
}

export function buildSteps(algo: AlgoId, input: Int32Array, U: number): SortStep[] {
  let gen: Generator<SortStep, Int32Array, void>;
  switch (algo) {
    case 'dial':
      gen = dialSortSteps(input, U);
      break;
    case 'radix':
      gen = radixSortLSDSteps(input);
      break;
    case 'merge':
      gen = mergeSortSteps(input);
      break;
  }
  const steps: SortStep[] = [];
  while (true) {
    const r = gen.next();
    if (r.done) break;
    steps.push(r.value);
  }
  return steps;
}
