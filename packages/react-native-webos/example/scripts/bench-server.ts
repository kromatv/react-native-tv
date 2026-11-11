/**
 * The machine side of `src/screens/bench`, whose method and results are in
 * `docs/benchmarks.md`. Package the bench entry with
 * `EXPO_PUBLIC_BENCH_REPORT_URL=http://<this machine>:47200`, install it, then:
 *
 * - `bun scripts/bench-server.ts cold-start <tv-ip> <app.tpk> [runs]` launches
 *   the app over sdb `runs` times (default 10). Each run times the launch
 *   command to the app's first-frame report, then answers it `exit` so the
 *   next launch starts a new process.
 * - `bun scripts/bench-server.ts listen` prints every report with the time it
 *   arrived, one JSON object per line, and lets the app run.
 */
import {
  SDB_PORT,
  SdbConnection,
  openPackage,
  readCapabilities,
  runShell,
} from '../../src/tizen/index.ts';

const PORT = Number(process.env['BENCH_PORT'] ?? 47200);
const FIRST_FRAME_TIMEOUT_MS = 60_000;
const EXIT_SETTLE_MS = Number(process.env['BENCH_SETTLE_MS'] ?? 8000);
const PID = /pid = (\d+)/;

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

async function coldStart(host: string, tpk: string, runs: number): Promise<void> {
  const { info } = await openPackage(tpk);
  exitOnFirstFrame = true;
  const times: number[] = [];
  for (let run = 1; run <= runs; run += 1) {
    const connection = await SdbConnection.open({ host, port: SDB_PORT });
    await readCapabilities(connection);
    const firstFrame = nextFirstFrame();
    const launchedAt = performance.now();
    const output = await runShell(connection, `0 execute ${info.appId}`, { timeoutMs: 60_000 });
    const launchReturnedMs = performance.now() - launchedAt;
    connection.close();
    const elapsedMs = (await firstFrame) - launchedAt;
    times.push(elapsedMs);
    const pid = PID.exec(output)?.[1] ?? 'unknown';
    console.log(
      JSON.stringify({
        run,
        pid,
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

const [mode, host, tpk, runs] = process.argv.slice(2);
if (mode === 'cold-start' && host !== undefined && tpk !== undefined) {
  await coldStart(host, tpk, Number(runs ?? 10));
} else if (mode !== 'listen') {
  console.error('Usage: bench-server.ts cold-start <tv-ip> <app.tpk> [runs] | listen');
  process.exit(2);
}
