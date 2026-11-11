import { afterEach, describe, expect, it, vi } from 'vitest';
import type { DevServerPoll } from './dev-server-ready.ts';
import { followJob } from './follow-job.ts';
import type { Job } from './jobs.ts';
import { openOnTizen } from './open-on-tizen.ts';
import { startPlannedJob } from './start-planned-job.ts';

vi.mock('./start-planned-job.ts', () => ({ startPlannedJob: vi.fn() }));
vi.mock('./follow-job.ts', () => ({ followJob: vi.fn() }));

const ORIGIN = 'http://localhost:8191';
const ENV = { EXPO_DEV_SERVER_ORIGIN: ORIGIN, EXPO_PUBLIC_PROJECT_ROOT: '/app' };

const job = (status: Job['status']): Job => ({
  id: 'job-1',
  kind: 'run-tv',
  title: 'Run on TV',
  args: [],
  cwd: '/app',
  status,
  startedAt: '2026-11-04T10:00:00.000Z',
});

const answering: DevServerPoll = {
  timeoutMs: 1000,
  intervalMs: 1,
  status: async () => 'packager-status:running',
  now: () => 0,
  sleep: async () => undefined,
};

async function open(choose: () => Promise<string | null>, finished: Job['status'] = 'succeeded') {
  vi.mocked(startPlannedJob).mockResolvedValue(job('running'));
  vi.mocked(followJob).mockResolvedValue(job(finished));
  let printed = '';
  await openOnTizen(choose, ENV, {
    write: (text) => {
      printed += text;
    },
    dir: '/jobs',
    poll: answering,
  });
  return printed;
}

afterEach(() => {
  vi.clearAllMocks();
});

describe('openOnTizen', () => {
  it('runs Run on TV against the dev server, then follows it', async () => {
    const printed = await open(async () => '10.0.0.7:26101');

    expect(startPlannedJob).toHaveBeenCalledWith(
      '/jobs',
      expect.objectContaining({
        kind: 'run-tv',
        args: ['run', '--device', '10.0.0.7:26101', '--dev-server', ORIGIN, '--no-logs'],
      }),
      '/app',
    );
    expect(followJob).toHaveBeenCalledWith('/jobs', 'job-1', expect.anything());
    expect(printed).toBe('› Opening on TV 10.0.0.7:26101...\n');
  });

  it('opens the simulator, and says how to try again when the run fails', async () => {
    const printed = await open(async () => 'simulator', 'failed');

    expect(vi.mocked(startPlannedJob).mock.calls[0]?.[1]).toMatchObject({ kind: 'run-simulator' });
    expect(printed).toContain('› Opening in the Tizen simulator...\n');
    expect(printed).toContain(
      '› Run in simulator did not finish: fix the step above, then press t again\n',
    );
  });

  it('opens nothing when the choice is cancelled, and prints why it could not open', async () => {
    expect(await open(async () => null)).toBe('');
    expect(await open(async () => Promise.reject(new Error('no network')))).toBe(
      '› rn-tizen could not start the command: no network\n',
    );
    expect(startPlannedJob).not.toHaveBeenCalled();
  });

  it("runs in Expo's working directory when Expo names no project root", async () => {
    vi.mocked(startPlannedJob).mockResolvedValue(job('running'));
    vi.mocked(followJob).mockResolvedValue(job('succeeded'));

    await openOnTizen(
      async () => 'simulator',
      { EXPO_DEV_SERVER_ORIGIN: ORIGIN },
      {
        write: () => undefined,
        dir: '/jobs',
        poll: answering,
      },
    );

    expect(vi.mocked(startPlannedJob).mock.calls[0]?.[2]).toBe(process.cwd());
  });
});
