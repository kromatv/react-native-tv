import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { singleReactNativeIssue } from '../dependencies/react-native-copies.ts';
import { FixableError } from '../errors.ts';
import { tizenMetroIssue } from '../metro/metro-platforms.ts';
import { openPlan } from './devtools-commands.ts';
import { readJob, startJob, writeJob, type Job } from './jobs.ts';
import { startPlannedJob } from './start-planned-job.ts';

vi.mock('../metro/metro-platforms.ts', () => ({ tizenMetroIssue: vi.fn(() => null) }));
vi.mock('../dependencies/react-native-copies.ts', () => ({
  singleReactNativeIssue: vi.fn(() => null),
}));
vi.mock('./jobs.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('./jobs.ts')>()),
  startJob: vi.fn(async (_dir: string, request: Pick<Job, 'kind'>) => ({ ...request, id: 'new' })),
}));

let dir = '';

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-planned-'));
});

afterEach(() => {
  vi.clearAllMocks();
  rmSync(dir, { recursive: true, force: true });
});

describe('openPlan', () => {
  it('runs Run on TV or Run in simulator against the dev server, ending at launch', () => {
    expect(openPlan('10.0.0.7:26101', 'http://localhost:8081')).toEqual({
      kind: 'run-tv',
      title: 'Run on TV 10.0.0.7:26101',
      args: [
        'run',
        '--device',
        '10.0.0.7:26101',
        '--dev-server',
        'http://localhost:8081',
        '--no-logs',
      ],
      exclusive: true,
      bundles: true,
    });
    expect(openPlan('simulator', 'http://localhost:8081').args).toEqual([
      'simulator',
      'run',
      '--dev-server',
      'http://localhost:8081',
      '--no-logs',
    ]);
  });
});

describe('startPlannedJob', () => {
  it('stops the running job of the same kind, then starts the new one', async () => {
    const running: Job = {
      id: 'old',
      kind: 'run-tv',
      title: 'Run on TV',
      args: [],
      cwd: dir,
      status: 'running',
      startedAt: '2026-11-04T10:00:00.000Z',
      pid: 2 ** 30,
    };
    await writeJob(dir, running);

    await startPlannedJob(dir, openPlan('10.0.0.7:26101', 'http://localhost:8081'), '/app');

    expect(startJob).toHaveBeenCalledWith(
      dir,
      expect.objectContaining({ kind: 'run-tv', cwd: '/app' }),
    );
    expect((await readJob(dir, 'old'))?.status).toBe('running');
  });

  it('refuses an app Metro cannot bundle for Tizen', async () => {
    vi.mocked(tizenMetroIssue).mockReturnValueOnce(new FixableError('no tizen', 'Run init.'));

    await expect(
      startPlannedJob(dir, openPlan('simulator', 'http://localhost:8081'), '/app'),
    ).rejects.toThrow('no tizen');
    expect(singleReactNativeIssue).toHaveBeenCalledWith('/app');
    expect(startJob).not.toHaveBeenCalled();
  });
});
