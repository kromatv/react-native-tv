import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { captureScreen } from '../capture/capture-screen.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { logsCommand } from './logs.ts';
import { perfCommand } from './perf.ts';
import { screenshotCommand } from './screenshot.ts';

vi.mock('../logs/log-server.ts', () => ({ listenForLogs: vi.fn() }));
vi.mock('../capture/capture-screen.ts', () => ({ captureScreen: vi.fn() }));
vi.mock('../cli/log-level-option.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../cli/log-level-option.ts')>()),
  untilInterrupted: vi.fn(async () => undefined),
}));

let dir: string;
let output: ReturnType<typeof captureProcessOutput>;
let server: LogServer;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-listen-'));
  output = captureProcessOutput();
  server = { port: 8102, capture: vi.fn(), close: vi.fn(async () => undefined) };
  vi.mocked(listenForLogs).mockResolvedValue(server);
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('rn-tizen logs', () => {
  it('listens on --port until Ctrl+C, then closes', async () => {
    await logsCommand.run(['--port', '8102', '--level', 'debug'], { cwd: dir });

    expect(listenForLogs).toHaveBeenCalledWith(8102, expect.anything(), { app: null });
    expect(output.stdout()).toMatch(/Listening on .+:8102 for every app\. Press Ctrl\+C to stop\./);
    expect(server.close).toHaveBeenCalled();
  });

  it('listens to the app --package names only', async () => {
    await logsCommand.run(['--package', 'com.example.tv'], { cwd: dir });

    expect(listenForLogs).toHaveBeenCalledWith(8099, expect.anything(), { app: 'com.example.tv' });
    expect(output.stdout()).toContain('for com.example.tv.');
    await expect(logsCommand.run(['--package', 'tv'], { cwd: dir })).rejects.toThrow(
      '--package is not a Tizen package id',
    );
  });

  it('refuses a level it does not know', async () => {
    await expect(logsCommand.run(['--level', 'loud'], { cwd: dir })).rejects.toThrow(
      '--level must be one of',
    );
  });
});

describe('rn-tizen perf', () => {
  it('listens only to the TV --device names, and saves what it heard to --out', async () => {
    await perfCommand.run(['--device', '192.168.1.50', '--out', 'boot.jsonl'], { cwd: dir });

    expect(listenForLogs).toHaveBeenCalledWith(8099, expect.anything(), {
      inUseHint: expect.stringContaining('rn-tizen perf listens on the log port itself'),
      peer: '192.168.1.50',
      app: null,
    });
    expect(readFileSync(join(dir, 'boot.jsonl'), 'utf8')).toBe('');
    expect(output.stdout()).toContain(`0 samples, saved to ${join(dir, 'boot.jsonl')}`);
  });

  it('refuses a duration that is not a number of seconds', async () => {
    await expect(perfCommand.run(['--duration', 'soon'], { cwd: dir })).rejects.toThrow(
      '--duration must be a number of seconds, got soon',
    );
  });
});

describe('rn-tizen screenshot', () => {
  it('saves the frame the app sends as a PNG at --out', async () => {
    vi.mocked(captureScreen).mockResolvedValue({
      width: 1,
      height: 1,
      stride: 4,
      pixels: Buffer.from([255, 0, 0, 255]),
    });

    await screenshotCommand.run(['--device', '192.168.1.50', '--out', 'shot.png'], { cwd: dir });

    expect(captureScreen).toHaveBeenCalledWith(8099, '192.168.1.50', null);
    await screenshotCommand.run(['--package', 'com.example.tv', '--out', 'shot.png'], { cwd: dir });
    expect(captureScreen).toHaveBeenLastCalledWith(8099, null, 'com.example.tv');
    expect(output.stdout()).toContain('com.example.tv, ');
    expect(readFileSync(join(dir, 'shot.png')).subarray(1, 4).toString()).toBe('PNG');
  });

  it('refuses a --device that is not an address', async () => {
    await expect(screenshotCommand.run(['--device', 'kitchen'], { cwd: dir })).rejects.toThrow(
      "--device must be the TV's IP address, got kitchen",
    );
  });
});
