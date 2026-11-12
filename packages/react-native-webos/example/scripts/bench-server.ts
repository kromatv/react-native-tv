/**
 * The machine side of `src/screens/bench`, whose method and results are in
 * `docs/benchmarks.md`. Package the bench entry with
 * `EXPO_PUBLIC_BENCH_REPORT_URL=http://<this machine>:47200`, install it, then:
 *
 * - `bun scripts/bench-server.ts cold-start <tv> <app id> [runs]` launches the
 *   app on a connected TV `runs` times (default 10). Each run times the launch
 *   request, its SSH login included, to the app's first-frame report, then
 *   answers it `exit` so the next launch starts a new process.
 * - `bun scripts/bench-server.ts listen` prints every report with the time it
 *   arrived, one JSON object per line, and lets the app run.
 */
import { launchApp, resolveDevice } from '../../src/webos/index.ts';

const PORT = Number(process.env['BENCH_PORT'] ?? 47200);
const FIRST_FRAME_TIMEOUT_MS = 60_000;
const EXIT_SETTLE_MS = Number(process.env['BENCH_SETTLE_MS'] ?? 8000);

type Waiter = (arrivedAt: number) => void;

let exitOnFirstFrame = false;
let firstFrameWaiter: Waiter | null = null;

Bun.serve({
  port: PORT,
  hostname: '0.0.0.0',
  async fetch(request) {
    const arrivedAt = performance.now();
    const event = new URL(request.url).pathname.slice(1);
    const body = await request.text();
    console.log(
      JSON.stringify({ at: new Date().toISOString(), event, data: JSON.parse(body || '{}') }),
    );
    if (event === 'first-frame') {
      firstFrameWaiter?.(arrivedAt);
      firstFrameWaiter = null;
      return new Response(exitOnFirstFrame ? 'exit' : 'stay');
    }
    return new Response('ok');
  },
});

function nextFirstFrame(): Promise<number> {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(
      () => reject(new Error('no first-frame report')),
      FIRST_FRAME_TIMEOUT_MS,
    );
    firstFrameWaiter = (arrivedAt) => {
      clearTimeout(timer);
      resolve(arrivedAt);
    };
  });
}

function median(values: readonly number[]): number {
  const sorted = [...values].sort((a, b) => a - b);
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2 === 1
    ? (sorted[middle] ?? 0)
    : ((sorted[middle - 1] ?? 0) + (sorted[middle] ?? 0)) / 2;
}

async function coldStart(tv: string, appId: string, runs: number): Promise<void> {
  const device = await resolveDevice(tv);
  exitOnFirstFrame = true;
  const times: number[] = [];
  for (let run = 1; run <= runs; run += 1) {
    const firstFrame = nextFirstFrame();
    const launchedAt = performance.now();
    await launchApp(device, appId);
    const launchReturnedMs = performance.now() - launchedAt;
    const elapsedMs = (await firstFrame) - launchedAt;
    times.push(elapsedMs);
    console.log(
      JSON.stringify({
        run,
        launchReturnedMs: Math.round(launchReturnedMs),
        firstFrameMs: Math.round(elapsedMs),
      }),
    );
    await Bun.sleep(EXIT_SETTLE_MS);
  }
  console.log(
    JSON.stringify({
      runs,
      medianMs: Math.round(median(times)),
      minMs: Math.round(Math.min(...times)),
      maxMs: Math.round(Math.max(...times)),
    }),
  );
  process.exit(0);
}

const [mode, tv, appId, runs] = process.argv.slice(2);
if (mode === 'cold-start' && tv !== undefined && appId !== undefined) {
  await coldStart(tv, appId, Number(runs ?? 10));
} else if (mode !== 'listen') {
  console.error('Usage: bench-server.ts cold-start <tv> <app id> [runs] | listen');
  process.exit(2);
}
