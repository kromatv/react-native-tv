import { spawn } from 'node:child_process';
import { existsSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { listJobs, readJob, readLog, startJob, stopJob, writeJob, type Job } from './jobs.ts';
import { runJobFromArgv } from './run-job.ts';

let dir = '';

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-jobs-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

const job = (id: string, status: Job['status'], startedAt = '2026-11-03T10:00:00.000Z'): Job => ({
  id,
  kind: 'find-tvs',
  title: 'Find TVs',
  args: [],
  cwd: dir,
  status,
  startedAt,
});

function script(name: string, source: string): string {
  const file = join(dir, name);
  writeFileSync(file, source);
  return file;
}

async function until<T>(
  read: () => Promise<T>,
  done: (value: T) => boolean,
  tries = 400,
): Promise<T> {
  const value = await read();
  if (done(value) || tries === 0) return value;
  await new Promise((resolve) => setTimeout(resolve, 25));
  return until(read, done, tries - 1);
}

function isAlive(pid: number): boolean {
  try {
    process.kill(pid, 0);
    return true;
  } catch {
    return false;
  }
}

const exited = (pid: number) =>
  until(
    () => Promise.resolve(isAlive(pid)),
    (alive) => !alive,
  );

describe('job records', () => {
  it('refuses records that are too large, not JSON or not a job', async () => {
    writeFileSync(join(dir, 'big-1.json'), ' '.repeat(65 * 1024));
    writeFileSync(join(dir, 'bad-1.json'), '{');
    writeFileSync(join(dir, 'odd-1.json'), '{"id":"odd-1"}');

    expect(await readJob(dir, 'big-1')).toBeNull();
    expect(await readJob(dir, 'bad-1')).toBeNull();
    expect(await readJob(dir, 'odd-1')).toBeNull();
    expect(await listJobs(dir)).toEqual([]);
    expect(await listJobs(join(dir, 'missing'))).toEqual([]);
    expect(await readLog(dir, 'none-1', 5)).toEqual({ text: '', next: 5 });
  });
});

describe('startJob', () => {
  it('starts a detached runner on the new job, and keeps the last 20 finished ones', async () => {
    const day = (n: number) => `2026-01-${String(n).padStart(2, '0')}T00:00:00.000Z`;
    await writeJob(dir, job('old-running', 'running', day(1)));
    await Promise.all(
      Array.from({ length: 21 }, (_, n) =>
        writeJob(dir, job(`old-${n + 2}`, 'succeeded', day(n + 2))),
      ),
    );
    const runner = script(
      'runner.mjs',
      "import { appendFileSync } from 'node:fs';\nconst [dir, id] = process.argv.slice(2);\nappendFileSync(`${dir}/${id}.log`, 'ran');\n",
    );

    const started = await startJob(
      dir,
      { kind: 'find-tvs', title: 'Find TVs', args: ['devices'], cwd: dir },
      runner,
    );
    const log = await until(
      () => readLog(dir, started.id, 0),
      (read) => read.text === 'ran',
    );

    expect(log.text).toBe('ran');
    expect(await readJob(dir, started.id)).toMatchObject({ status: 'running', args: ['devices'] });
    expect(existsSync(join(dir, 'old-2.json'))).toBe(false);
    expect(existsSync(join(dir, 'old-3.json'))).toBe(true);
    expect(existsSync(join(dir, 'old-running.json'))).toBe(true);
  });
});

describe('stopJob', () => {
  it('stops a running job and its process group', async () => {
    const child = spawn(process.execPath, ['-e', 'setInterval(() => {}, 1000)'], {
      detached: true,
      stdio: 'ignore',
    });
    const { pid } = child;
    if (pid === undefined) throw new Error('node did not start');
    await writeJob(dir, { ...job('live-1', 'running'), pid });

    const stopped = await stopJob(dir, 'live-1');
    await exited(pid);

    expect(stopped?.status).toBe('stopped');
    expect(isAlive(pid)).toBe(false);
    expect((await readJob(dir, 'live-1'))?.status).toBe('stopped');
  });

  it('leaves a job that ended, one without a process, and one that does not exist', async () => {
    const gone = spawn(process.execPath, ['-e', ''], { stdio: 'ignore' });
    await new Promise((resolve) => gone.on('close', resolve));
    await writeJob(dir, { ...job('gone-1', 'running'), pid: gone.pid });
    await writeJob(dir, job('done-1', 'succeeded'));
    await writeJob(dir, job('nopid-1', 'running'));

    expect((await stopJob(dir, 'gone-1'))?.status).toBe('running');
    expect((await stopJob(dir, 'done-1'))?.status).toBe('succeeded');
    expect((await stopJob(dir, 'nopid-1'))?.status).toBe('running');
    expect(await stopJob(dir, 'none-1')).toBeNull();
  });
});

describe('runJob', () => {
  const CLI = [
    "import { readFileSync, writeFileSync } from 'node:fs';",
    'const [mode, record] = process.argv.slice(2);',
    "console.log('hello ' + mode);",
    "if (mode === 'stop') writeFileSync(record, JSON.stringify({ ...JSON.parse(readFileSync(record, 'utf8')), status: 'stopped' }));",
    "process.exitCode = mode === 'fail' ? 3 : 0;",
  ].join('\n');

  it.each([
    ['ok', 'succeeded', 0],
    ['fail', 'failed', 3],
    ['stop', 'stopped', null],
  ])('records a run that ends with %s as %s', async (mode, status, exitCode) => {
    const cli = script('cli.mjs', CLI);
    const id = `${mode}-1`;
    await writeJob(dir, { ...job(id, 'running'), args: [mode, join(dir, `${id}.json`)] });

    await runJobFromArgv([dir, id], cli);

    const record = await readJob(dir, id);

    expect(record?.status).toBe(status);
    expect(record?.exitCode ?? null).toBe(exitCode);
    expect((await readLog(dir, id, 0)).text).toBe(`hello ${mode}\n`);
  });

  it('does nothing for a job that does not exist, or without one named', async () => {
    const cli = script('cli.mjs', CLI);
    await runJobFromArgv([dir, 'none-1'], cli);
    await runJobFromArgv([dir], cli);

    expect(existsSync(join(dir, 'none-1.log'))).toBe(false);
  });
});
