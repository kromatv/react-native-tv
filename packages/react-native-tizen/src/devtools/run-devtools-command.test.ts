import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { FixableError } from '../errors.ts';
import { tizenMetroIssue } from '../metro/metro-platforms.ts';
import { jobPageUrl } from './devtools-commands.ts';
import { startJob, writeJob, type Job } from './jobs.ts';
import { runDevtoolsCommand } from './run-devtools-command.ts';

vi.mock('../metro/metro-platforms.ts', () => ({ tizenMetroIssue: vi.fn(() => null) }));
vi.mock('./jobs.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('./jobs.ts')>()),
  startJob: vi.fn(),
}));

const DEV_SERVER = 'http://192.168.1.20:8081';
let dir = '';

const job = (id: string, kind: string): Job => ({
  id,
  kind,
  title: 'Run in simulator',
  args: [],
  cwd: dir,
  status: 'running',
  startedAt: '2026-11-03T10:00:00.000Z',
});

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-devtools-command-'));
});

afterEach(() => {
  vi.resetAllMocks();
  rmSync(dir, { recursive: true, force: true });
});

describe('runDevtoolsCommand', () => {
  it('starts the job and points at its page', async () => {
    await writeJob(dir, job('last-1', 'run-simulator'));
    vi.mocked(startJob).mockResolvedValueOnce(job('new-1', 'run-simulator'));

    const result = await runDevtoolsCommand(['run-simulator', '{}', DEV_SERVER], dir);

    expect(result).toEqual({
      exitCode: 0,
      lines: [
        {
          type: 'text',
          level: 'info',
          text: 'Run in simulator started; progress and logs',
          url: jobPageUrl(DEV_SERVER, 'new-1'),
        },
      ],
    });
    expect(startJob).toHaveBeenCalledWith(dir, {
      kind: 'run-simulator',
      title: 'Run in simulator',
      args: ['simulator', 'run', '--dev-server', DEV_SERVER],
      cwd: process.cwd(),
    });
  });

  it.each([
    ['an unknown command', ['nope', '{}', DEV_SERVER], 'Unknown command nope'],
    ['missing arguments', ['find-tvs'], 'expected'],
    ['arguments that are not JSON', ['find-tvs', '{', DEV_SERVER], 'could not start the command'],
  ])('answers %s with one error line', async (_, argv, message) => {
    const { lines, exitCode } = await runDevtoolsCommand(argv, dir);

    expect(exitCode).toBe(1);
    expect(lines).toEqual([expect.objectContaining({ level: 'error' })]);
    expect(lines[0]?.text).toContain(message);
  });

  it('refuses to run the app when its Metro config has no withTizen, and says how to fix it', async () => {
    vi.mocked(tizenMetroIssue).mockReturnValueOnce(
      new FixableError(
        "The app's Metro config does not know the tizen platform",
        'Run npx rn-tizen init',
      ),
    );

    const { lines, exitCode } = await runDevtoolsCommand(['run-simulator', '{}', DEV_SERVER], dir);
    await runDevtoolsCommand(['find-tvs', '{}', DEV_SERVER], dir);

    expect(exitCode).toBe(1);
    expect(lines[0]?.text).toBe(
      "The app's Metro config does not know the tizen platform. Run npx rn-tizen init",
    );
    expect(tizenMetroIssue).toHaveBeenCalledOnce();
    expect(startJob).toHaveBeenCalledOnce();
  });

  it('says why a job could not start', async () => {
    vi.mocked(startJob).mockRejectedValueOnce(new Error('disk full'));
    const first = await runDevtoolsCommand(['find-tvs', '{}', DEV_SERVER], dir);
    vi.mocked(startJob).mockRejectedValueOnce('no reason');
    const second = await runDevtoolsCommand(['find-tvs', '{}', DEV_SERVER], dir);

    expect(first.lines[0]?.text).toBe('rn-tizen could not start the command: disk full');
    expect(second.lines[0]?.text).toBe('rn-tizen could not start the command: no reason');
  });
});
