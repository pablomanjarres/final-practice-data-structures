import { useEffect, useMemo, useRef, useState } from 'react';
import {
  ALGO_BIG_O,
  ALGO_LABELS,
  DISTRIBUTION_LABELS,
  buildSteps,
  generateInput,
} from './algorithms';
import type { AlgoId, Distribution, SortStep } from './algorithms';

const W = 1000;
const H = 520;
const PAD = 16;

const COLORS = {
  bg: '#0e1116',
  surface: '#161b22',
  border: '#2a3340',
  text: '#e6edf3',
  muted: '#8b949e',
  bar: '#475569',
  bucket: '#3a4a5e',
  bucketActive: '#fbbf24',
  output: '#34d399',
  highlight: '#f59e0b',
  read: '#f87171',
  range: 'rgba(125, 211, 252, 0.18)',
  ptrI: '#fbbf24',
  ptrJ: '#a78bfa',
};

export function Visualizer() {
  const [algo, setAlgo] = useState<AlgoId>('dial');
  const [size, setSize] = useState(32);
  const [dist, setDist] = useState<Distribution>('uniform');
  const [seed, setSeed] = useState(42);
  const [speed, setSpeed] = useState(4);
  const [stepIdx, setStepIdx] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);

  const U = size; // visualizer keeps it simple: U = n
  const input = useMemo(() => generateInput(dist, size, U, seed), [dist, size, U, seed]);
  const steps = useMemo(() => buildSteps(algo, input, U), [algo, input, U]);

  useEffect(() => {
    setStepIdx(0);
    setIsPlaying(false);
  }, [steps]);

  const playingRef = useRef(false);
  playingRef.current = isPlaying;
  const speedRef = useRef(speed);
  speedRef.current = speed;

  useEffect(() => {
    if (!isPlaying) return;
    let raf = 0;
    const tick = () => {
      if (!playingRef.current) return;
      setStepIdx((idx) => {
        const next = idx + speedRef.current;
        if (next >= steps.length - 1) {
          playingRef.current = false;
          setIsPlaying(false);
          return steps.length - 1;
        }
        return next;
      });
      raf = requestAnimationFrame(tick);
    };
    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [isPlaying, steps]);

  const canvasRef = useRef<HTMLCanvasElement>(null);
  useEffect(() => {
    const cvs = canvasRef.current;
    if (!cvs) return;
    const ctx = cvs.getContext('2d');
    if (!ctx) return;
    const dpr = window.devicePixelRatio || 1;
    cvs.width = W * dpr;
    cvs.height = H * dpr;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.fillStyle = COLORS.bg;
    ctx.fillRect(0, 0, W, H);
    const idx = Math.min(stepIdx, steps.length - 1);
    if (algo === 'dial') renderDial(ctx, input, U, steps, idx);
    else if (algo === 'radix') renderRadix(ctx, input, steps, idx);
    else renderMerge(ctx, input, steps, idx);
  }, [algo, input, U, steps, stepIdx]);

  const handlePlay = () => {
    if (stepIdx >= steps.length - 1) setStepIdx(0);
    setIsPlaying(true);
  };

  return (
    <div className="grid">
      <aside className="controls">
        <h2>{ALGO_LABELS[algo]}</h2>
        <p className="bigo">
          Complexity: <code>{ALGO_BIG_O[algo]}</code>
        </p>

        <label>Algorithm</label>
        <div className="row">
          {(['dial', 'radix', 'merge'] as AlgoId[]).map((a) => (
            <button
              key={a}
              className={`chip ${algo === a ? 'active' : ''}`}
              onClick={() => setAlgo(a)}
            >
              {ALGO_LABELS[a].split(' ')[0]}
            </button>
          ))}
        </div>

        <label>Input size n = {size}</label>
        <input
          type="range"
          min={8}
          max={128}
          value={size}
          onChange={(e) => setSize(Number(e.target.value))}
        />

        <label>Distribution</label>
        <select value={dist} onChange={(e) => setDist(e.target.value as Distribution)}>
          {(Object.keys(DISTRIBUTION_LABELS) as Distribution[]).map((d) => (
            <option key={d} value={d}>
              {DISTRIBUTION_LABELS[d]}
            </option>
          ))}
        </select>

        <label>Random seed</label>
        <input
          type="number"
          value={seed}
          onChange={(e) => setSeed(Number(e.target.value))}
        />

        <label>Speed: {speed} step{speed === 1 ? '' : 's'}/frame</label>
        <input
          type="range"
          min={1}
          max={32}
          value={speed}
          onChange={(e) => setSpeed(Number(e.target.value))}
        />

        <div className="row">
          {!isPlaying ? (
            <button className="primary" onClick={handlePlay}>
              ▶ Play
            </button>
          ) : (
            <button className="primary" onClick={() => setIsPlaying(false)}>
              ❚❚ Pause
            </button>
          )}
          <button onClick={() => setStepIdx((i) => Math.max(0, i - 1))} disabled={stepIdx <= 0}>
            ◀
          </button>
          <button
            onClick={() => setStepIdx((i) => Math.min(steps.length - 1, i + 1))}
            disabled={stepIdx >= steps.length - 1}
          >
            ▶
          </button>
          <button onClick={() => { setIsPlaying(false); setStepIdx(0); }}>↺</button>
        </div>

        <p className="muted small">
          Step {Math.min(stepIdx + 1, steps.length)} / {steps.length}
        </p>

        <p className="footnote">
          Canonical implementation + benchmark in <code>cpp/</code>. Run{' '}
          <code>cd cpp &amp;&amp; make &amp;&amp; ./bench --quick</code>.
        </p>
      </aside>

      <canvas ref={canvasRef} className="viz" style={{ aspectRatio: `${W} / ${H}` }} />
    </div>
  );
}

// ---------- rendering ----------

function maxOf(arr: ArrayLike<number>): number {
  let m = 1;
  for (let i = 0; i < arr.length; i++) if (arr[i] > m) m = arr[i];
  return m;
}

function drawArrayBars(
  ctx: CanvasRenderingContext2D,
  arr: ArrayLike<number>,
  x0: number,
  y0: number,
  width: number,
  height: number,
  maxVal: number,
  highlight: number,
  highlightColor: string
) {
  const n = arr.length;
  if (n === 0) return;
  const barW = width / n;
  for (let i = 0; i < n; i++) {
    const v = arr[i];
    const h = Math.max(2, (v / maxVal) * (height - 2));
    ctx.fillStyle = i === highlight ? highlightColor : COLORS.bar;
    ctx.fillRect(x0 + i * barW + 0.5, y0 + height - h, Math.max(1, barW - 1), h);
  }
}

function drawCellGrid(
  ctx: CanvasRenderingContext2D,
  arr: ArrayLike<number>,
  x0: number,
  y0: number,
  width: number,
  height: number,
  maxVal: number,
  highlight: number,
  filledLen: number
) {
  const n = arr.length;
  if (n === 0) return;
  const barW = width / n;
  for (let i = 0; i < n; i++) {
    if (i >= filledLen) {
      ctx.fillStyle = COLORS.surface;
      ctx.fillRect(x0 + i * barW + 0.5, y0, Math.max(1, barW - 1), height);
      ctx.strokeStyle = COLORS.border;
      ctx.strokeRect(x0 + i * barW + 0.5, y0, Math.max(1, barW - 1), height);
      continue;
    }
    const v = arr[i];
    const h = Math.max(2, (v / maxVal) * (height - 2));
    ctx.fillStyle = i === highlight ? COLORS.highlight : COLORS.output;
    ctx.fillRect(x0 + i * barW + 0.5, y0 + height - h, Math.max(1, barW - 1), h);
  }
}

function label(ctx: CanvasRenderingContext2D, text: string, x: number, y: number, color = COLORS.muted) {
  ctx.fillStyle = color;
  ctx.font = '11px -apple-system, BlinkMacSystemFont, system-ui, sans-serif';
  ctx.textBaseline = 'top';
  ctx.fillText(text, x, y);
}

function title(ctx: CanvasRenderingContext2D, text: string, x: number, y: number) {
  ctx.fillStyle = COLORS.text;
  ctx.font = 'bold 12px -apple-system, BlinkMacSystemFont, system-ui, sans-serif';
  ctx.textBaseline = 'top';
  ctx.fillText(text, x, y);
}

function renderDial(
  ctx: CanvasRenderingContext2D,
  input: Int32Array,
  U: number,
  steps: SortStep[],
  idx: number
) {
  const buckets: number[][] = Array.from({ length: U }, () => []);
  const output: number[] = new Array(input.length).fill(0);
  let outputLen = 0;
  let hSrc = -1, hBucket = -1, hOut = -1;
  let status = 'Ready.';
  for (let k = 0; k <= idx && k < steps.length; k++) {
    const s = steps[k];
    if (s.kind === 'read') { hSrc = s.i; status = `Read input[${s.i}] = ${input[s.i]}`; }
    else if (s.kind === 'bucketPush') {
      buckets[s.bucket].push(s.v); hBucket = s.bucket; hOut = -1;
      status = `Push ${s.v} → bucket[${s.bucket}]`;
    }
    else if (s.kind === 'bucketEmit') {
      output[s.destIdx] = s.v; outputLen = Math.max(outputLen, s.destIdx + 1);
      hBucket = s.bucket; hOut = s.destIdx; hSrc = -1;
      status = `Emit ${s.v} from bucket[${s.bucket}] → output[${s.destIdx}]`;
    } else if (s.kind === 'done') {
      hSrc = hBucket = hOut = -1;
      status = `Done — ${outputLen} elements sorted.`;
    }
  }

  const maxV = U - 1;
  title(ctx, 'Source array (input)', PAD, PAD);
  drawArrayBars(ctx, input, PAD, PAD + 18, W - 2 * PAD, 96, maxV, hSrc, COLORS.read);

  title(ctx, `Buckets (universe size U = ${U})`, PAD, PAD + 130);
  const bx = PAD, by = PAD + 148, bw = W - 2 * PAD, bh = 240;
  ctx.fillStyle = COLORS.surface;
  ctx.fillRect(bx, by, bw, bh);
  ctx.strokeStyle = COLORS.border;
  ctx.strokeRect(bx, by, bw, bh);

  let maxFill = 1;
  for (let b = 0; b < U; b++) if (buckets[b].length > maxFill) maxFill = buckets[b].length;
  const colW = bw / U;
  for (let b = 0; b < U; b++) {
    const fill = buckets[b].length;
    if (fill === 0) continue;
    const h = (fill / maxFill) * (bh - 8);
    ctx.fillStyle = b === hBucket ? COLORS.bucketActive : COLORS.bucket;
    ctx.fillRect(bx + b * colW + 0.5, by + bh - h - 2, Math.max(1, colW - 1), h);
  }
  label(ctx, '0', bx, by + bh + 4);
  label(ctx, String(U - 1), bx + bw - 30, by + bh + 4);

  title(ctx, 'Output (sorted, fills left→right)', PAD, PAD + 410);
  drawCellGrid(ctx, output, PAD, PAD + 428, W - 2 * PAD, 72, maxV, hOut, outputLen);
  label(ctx, status, PAD, H - 18, COLORS.text);
}

function renderRadix(
  ctx: CanvasRenderingContext2D,
  input: Int32Array,
  steps: SortStep[],
  idx: number
) {
  let current = new Int32Array(input);
  let digit = 0;
  const counts = new Int32Array(256);
  let hSrc = -1, hDst = -1, hBucket = -1;
  let status = 'Ready.';
  let phase: 'count' | 'emit' | 'done' = 'count';

  for (let k = 0; k <= idx && k < steps.length; k++) {
    const s = steps[k];
    if (s.kind === 'read') hSrc = s.i;
    else if (s.kind === 'radixCount') {
      digit = s.digit; counts[s.bucket]++; hBucket = s.bucket; hSrc = s.sourceIdx; phase = 'count';
      status = `Pass d=${s.digit + 1}/4: count digit-byte ${s.bucket} (value ${s.v})`;
    } else if (s.kind === 'radixEmit') {
      hDst = s.destIdx; hBucket = s.bucket; phase = 'emit';
      status = `Pass d=${s.digit + 1}/4: emit ${s.v} → out[${s.destIdx}]`;
    } else if (s.kind === 'radixPass') {
      current = new Int32Array(s.snapshot);
      digit = s.digit + 1;
      counts.fill(0);
      hSrc = hDst = hBucket = -1;
      status = `Completed pass d=${s.digit + 1}/4`;
    } else if (s.kind === 'done') {
      phase = 'done';
      hSrc = hDst = hBucket = -1;
      status = 'Done — all 4 passes complete.';
    }
  }

  const maxV = maxOf(input);
  title(
    ctx,
    `Current array — Pass ${Math.min(digit + 1, 4)}/4, byte ${digit * 8}–${digit * 8 + 7}`,
    PAD, PAD
  );
  drawArrayBars(
    ctx,
    current,
    PAD,
    PAD + 18,
    W - 2 * PAD,
    140,
    maxV,
    phase === 'count' ? hSrc : hDst,
    phase === 'count' ? COLORS.read : COLORS.highlight
  );

  title(ctx, 'Bucket counts (256 digit values)', PAD, PAD + 180);
  const bx = PAD, by = PAD + 198, bw = W - 2 * PAD, bh = 240;
  ctx.fillStyle = COLORS.surface;
  ctx.fillRect(bx, by, bw, bh);
  ctx.strokeStyle = COLORS.border;
  ctx.strokeRect(bx, by, bw, bh);

  const colW = bw / 256;
  let maxFill = 1;
  for (let b = 0; b < 256; b++) if (counts[b] > maxFill) maxFill = counts[b];
  for (let b = 0; b < 256; b++) {
    if (counts[b] === 0) continue;
    const h = (counts[b] / maxFill) * (bh - 8);
    ctx.fillStyle = b === hBucket ? '#a78bfa' : '#5b4f8a';
    ctx.fillRect(bx + b * colW + 0.5, by + bh - h - 2, Math.max(1, colW - 1), h);
  }
  label(ctx, '0', bx, by + bh + 4);
  label(ctx, '255', bx + bw - 22, by + bh + 4);

  label(ctx, status, PAD, H - 18, COLORS.text);
}

function renderMerge(
  ctx: CanvasRenderingContext2D,
  input: Int32Array,
  steps: SortStep[],
  idx: number
) {
  const current = new Int32Array(input);
  let range: { lo: number; mid: number; hi: number } | null = null;
  let activeI = -1, activeJ = -1, lastWriteK = -1;
  let compares = 0, writes = 0;
  let status = 'Ready.';

  for (let k = 0; k <= idx && k < steps.length; k++) {
    const s = steps[k];
    if (s.kind === 'mergeRange') {
      range = { lo: s.lo, mid: s.mid, hi: s.hi };
      status = `Merge [${s.lo}…${s.mid}] ∪ [${s.mid + 1}…${s.hi}]`;
    } else if (s.kind === 'compare') {
      compares++; activeI = s.i; activeJ = s.j;
      status = `Compare a[${s.i}]=${current[s.i]} vs a[${s.j}]=${current[s.j]}`;
    } else if (s.kind === 'write') {
      writes++; current[s.i] = s.v; lastWriteK = s.i;
      status = `Write a[${s.i}] := ${s.v}`;
    } else if (s.kind === 'done') {
      range = null; activeI = activeJ = -1;
      status = `Done — ${compares} compares, ${writes} writes.`;
    }
  }

  const maxV = maxOf(input);
  const n = current.length;
  const ax = PAD, ay = PAD + 18, aw = W - 2 * PAD, ah = 320;
  title(ctx, 'Working array (sorted ranges merge into larger ranges)', PAD, PAD);

  if (range) {
    const barW = aw / n;
    const xLo = ax + range.lo * barW;
    const xHi = ax + (range.hi + 1) * barW;
    ctx.fillStyle = COLORS.range;
    ctx.fillRect(xLo, ay, xHi - xLo, ah);
    const xMid = ax + (range.mid + 1) * barW;
    ctx.strokeStyle = COLORS.muted;
    ctx.setLineDash([3, 3]);
    ctx.beginPath();
    ctx.moveTo(xMid, ay);
    ctx.lineTo(xMid, ay + ah);
    ctx.stroke();
    ctx.setLineDash([]);
  }

  const barW = aw / n;
  for (let i = 0; i < n; i++) {
    const v = current[i];
    const h = Math.max(2, (v / maxV) * (ah - 2));
    let color: string = COLORS.bar;
    if (i === activeI) color = COLORS.ptrI;
    else if (i === activeJ) color = COLORS.ptrJ;
    else if (i === lastWriteK) color = COLORS.output;
    ctx.fillStyle = color;
    ctx.fillRect(ax + i * barW + 0.5, ay + ah - h, Math.max(1, barW - 1), h);
  }

  const sy = PAD + 380;
  title(ctx, 'Operation counts', PAD, sy);
  label(ctx, `Compares: ${compares}`, PAD, sy + 20, COLORS.text);
  label(ctx, `Writes:   ${writes}`, PAD + 140, sy + 20, COLORS.text);
  label(ctx, `Step:     ${idx + 1} / ${steps.length}`, PAD + 280, sy + 20, COLORS.text);

  label(ctx, status, PAD, H - 18, COLORS.text);
}
