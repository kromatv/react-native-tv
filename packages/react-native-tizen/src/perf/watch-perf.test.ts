import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { untilInterrupted } from '../cli/log-level-option.ts';
import { Reporter } from '../cli/reporter.ts';
import { captureProcessOutput, capturedOutput } from '../fakes.fixtures.ts';
import { listenForLogs, type LogListener, type LogServer } from '../logs/log-server.ts';
import { idleSample } from './perf-sample.fixtures.ts';
import { watchPerf } from './watch-perf.ts';

vi.mock('../logs/log-server.ts', () => ({ listenForLogs: vi.fn() }));
vi.mock('../cli/log-level-option.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../cli/log-level-option.ts')>()),
  untilInterrupted: vi.fn(),
}));

const WATCH = { out: null, minimum: 'warn', port: 8102, device: null, inUseHint: 'busy' } as const;

let listener: LogListener | null;
let server: LogServer;

function sample(frames: number) {
  listener?.onEntry(
    { level: 'debug', message: `perf ${JSON.stringify(idleSample({ frames }))}` },
    '192.168.1.50',
  );
}

function interruptible(): () => void {
  let interrupt: () => void = () => undefined;
  vi.mocked(untilInterrupted).mockReturnValue(
    new Promise((resolve) => {
      interrupt = () => resolve();
    }),
  );
  return () => interrupt();
}

beforeEach(() => {
  captureProcessOutput();
  listener = null;
  server = { port: 8102, capture: vi.fn(), close: vi.fn(async () => undefined) };
  vi.mocked(listenForLogs).mockImplementation(async (_port, heard) => {
    listener = heard;
    return server;
  });
});

afterEach(() => {
  vi.useRealTimers();
  vi.clearAllMocks();
});

describe('watchPerf', () => {
  it('stops the duration after the first sample, without Ctrl+C', async () => {
    vi.mocked(untilInterrupted).mockReturnValue(new Promise(() => undefined));
    const out = capturedOutput();

    const watching = watchPerf(new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } }), {
      ...WATCH,
      durationMs: 20,
    });
    await vi.waitFor(() => expect(listener).not.toBeNull());
    sample(60);
    const samples = await watching;

    expect(samples.map((entry) => entry.frames)).toEqual([60]);
    expect(out.text()).toContain('Stopping 0.02 s after the first sample.');
    expect(out.text()).toContain('1 samples');
    expect(server.close).toHaveBeenCalled();
  });

  it('says why nothing arrives when no sample came within ten seconds', async () => {
    vi.useFakeTimers({ toFake: ['setTimeout', 'clearTimeout'] });
    const interrupt = interruptible();
    const out = capturedOutput();

    const watching = watchPerf(new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } }), {
      ...WATCH,
      durationMs: null,
    });
    await vi.advanceTimersByTimeAsync(10_000);
    interrupt();
    await watching;

    expect(out.text()).toContain('No sample yet. A development build packaged with port 8102');
  });

  it('stays quiet at ten seconds once samples arrive', async () => {
    vi.useFakeTimers({ toFake: ['setTimeout', 'clearTimeout'] });
    const interrupt = interruptible();
    const out = capturedOutput();

    const watching = watchPerf(new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } }), {
      ...WATCH,
      durationMs: null,
    });
    await vi.advanceTimersByTimeAsync(0);
    sample(30);
    await vi.advanceTimersByTimeAsync(10_000);
    interrupt();
    await watching;

    expect(out.text()).not.toContain('No sample yet');
  });
});
