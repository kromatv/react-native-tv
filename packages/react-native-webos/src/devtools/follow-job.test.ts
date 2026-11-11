import { appendFileSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { followJob } from './follow-job.ts';
import { logPath, writeJob, type Job } from './jobs.ts';

let dir = '';

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-follow-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

const job = (status: Job['status'], pid?: number): Job => ({
  id: 'job-1',
  kind: 'run-tv',
  title: 'Run on TV',
  args: [],
  cwd: dir,
  status,
  startedAt: '2026-11-04T10:00:00.000Z',
  ...(pid === undefined ? {} : { pid }),
});

describe('followJob', () => {
  it('writes what the job prints as it prints it, then how it ended', async () => {
    await writeJob(dir, job('running', 2 ** 30));
    appendFileSync(logPath(dir, 'job-1'), '✓ Install\n');
    const written: string[] = [];
    let ticks = 0;

    const ended = await followJob(dir, 'job-1', {
      write: (text) => written.push(text),
      intervalMs: 1,
      sleep: async () => {
        ticks += 1;
        if (ticks === 1) appendFileSync(logPath(dir, 'job-1'), '✓ Launch\n');
        if (ticks === 2) await writeJob(dir, job('succeeded'));
      },
    });

    expect(written.join('')).toBe('✓ Install\n✓ Launch\n');
    expect(ended?.status).toBe('succeeded');
  });

  it('stops the job when Expo exits first, and lets it be once followed', async () => {
    await writeJob(dir, job('running', 2 ** 30));
    let stop: (() => void) | undefined;
    let cancelled = false;

    await followJob(dir, 'job-1', {
      write: () => undefined,
      sleep: async () => {
        stop?.();
        await writeJob(dir, job('failed'));
      },
      onExit: (callback) => {
        stop = callback;
        return () => {
          cancelled = true;
        };
      },
    });

    expect(cancelled).toBe(true);
    expect(() => stop?.()).not.toThrow();
  });
});
