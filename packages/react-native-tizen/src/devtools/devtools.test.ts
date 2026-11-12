import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { rememberDevice } from '../tizen/known-devices.ts';
import handler from './devtools-server.ts';
import { CommandInputError, jobPageUrl, planCommand } from './devtools-commands.ts';
import { jobsDir, listJobs, readJob, readLog, writeJob, type Job } from './jobs.ts';

let state = '';
const saved: Record<string, string | undefined> = {};

beforeEach(() => {
  state = mkdtempSync(join(tmpdir(), 'rn-tizen-devtools-'));
  for (const name of ['XDG_STATE_HOME', 'RN_TIZEN_CACHE_DIR']) saved[name] = process.env[name];
  process.env.XDG_STATE_HOME = join(state, 'state');
  process.env.RN_TIZEN_CACHE_DIR = join(state, 'cache');
});

afterEach(() => {
  for (const [name, value] of Object.entries(saved)) {
    if (value === undefined) delete process.env[name];
    else process.env[name] = value;
  }
  rmSync(state, { recursive: true, force: true });
});

const DEV_SERVER = 'http://192.168.1.20:8081';

describe('planCommand', () => {
  it('runs on the TV given, from the dev server', async () => {
    const plan = await planCommand('run-tv', { device: ' 192.168.1.50 ' }, DEV_SERVER);

    expect(plan.args).toEqual(['run', '--device', '192.168.1.50', '--dev-server', DEV_SERVER]);
    expect(plan.exclusive).toBe(true);
  });

  it('runs on the last TV used when no address is given, and says when there is none', async () => {
    await expect(planCommand('run-tv', { device: '' }, DEV_SERVER)).rejects.toBeInstanceOf(
      CommandInputError,
    );

    await rememberDevice({ host: '192.168.1.60', port: 26101 });

    expect((await planCommand('run-tv', { device: '' }, DEV_SERVER)).args).toContain(
      '192.168.1.60:26101',
    );
  });

  it('maps the other commands, and refuses an unknown one or extra arguments', async () => {
    expect((await planCommand('run-simulator', {}, DEV_SERVER)).args).toEqual([
      'simulator',
      'run',
      '--dev-server',
      DEV_SERVER,
    ]);
    expect((await planCommand('find-tvs', {}, DEV_SERVER)).args).toEqual(['devices']);
    expect((await planCommand('create-certificate', { name: 'ci' }, DEV_SERVER)).args).toEqual([
      'certificate',
      'create-author',
      'ci',
    ]);
    await expect(planCommand('find-tvs', { extra: 1 }, DEV_SERVER)).rejects.toThrow();
    await expect(planCommand('nope', {}, DEV_SERVER)).rejects.toThrow('Unknown command nope');
  });

  it('points at the plugin page inside the dev server', () => {
    expect(jobPageUrl(DEV_SERVER, 'abc-1')).toBe(
      'http://192.168.1.20:8081/_expo/plugins/@kromatv/react-native-tizen/?job=abc-1',
    );
  });
});

function job(id: string, startedAt: string): Job {
  return {
    id,
    kind: 'find-tvs',
    title: 'Find TVs',
    args: ['devices'],
    cwd: state,
    status: 'succeeded',
    startedAt,
  };
}

describe('jobs and the page API', () => {
  it('lists jobs newest first and reads a log from an offset', async () => {
    const dir = jobsDir();
    await import('node:fs/promises').then(({ mkdir }) => mkdir(dir, { recursive: true }));
    await writeJob(dir, job('a-1', '2026-11-03T10:00:00.000Z'));
    await writeJob(dir, job('b-2', '2026-11-03T11:00:00.000Z'));
    writeFileSync(join(dir, 'b-2.log'), 'hello world');

    expect((await listJobs(dir)).map((entry) => entry.id)).toEqual(['b-2', 'a-1']);
    expect(await readLog(dir, 'b-2', 6)).toEqual({ text: 'world', next: 11 });
    expect(await readJob(dir, '../etc')).toBeNull();
  });

  it('answers the page, and leaves every other path to the static files', async () => {
    const dir = jobsDir();
    await import('node:fs/promises').then(({ mkdir }) => mkdir(dir, { recursive: true }));
    await writeJob(dir, job('a-1', '2026-11-03T10:00:00.000Z'));
    writeFileSync(join(dir, 'a-1.log'), 'done');

    const list = await handler(new Request('http://x/api/jobs'));
    const log = await handler(new Request('http://x/api/jobs/a-1/log?from=0'));
    const stop = await handler(new Request('http://x/api/jobs/a-1/stop'));

    expect(await list?.json()).toMatchObject({ jobs: [{ id: 'a-1' }] });
    expect(await log?.json()).toMatchObject({ text: 'done', next: 4 });
    expect(stop?.status).toBe(405);
    expect(await handler(new Request('http://x/'))).toBeUndefined();
    expect((await handler(new Request('http://x/api/jobs/zz/log')))?.status).toBe(404);
  });

  it('stops a job on POST, and reads a log from the start for a bad offset', async () => {
    const dir = jobsDir();
    await import('node:fs/promises').then(({ mkdir }) => mkdir(dir, { recursive: true }));
    await writeJob(dir, job('a-1', '2026-11-03T10:00:00.000Z'));
    writeFileSync(join(dir, 'a-1.log'), 'done');

    const stop = await handler(new Request('http://x/api/jobs/a-1/stop', { method: 'POST' }));
    const log = await handler(new Request('http://x/api/jobs/a-1/log?from=-4'));
    const whole = await handler(new Request('http://x/api/jobs/a-1/log'));

    expect(await stop?.json()).toMatchObject({ job: { id: 'a-1', status: 'succeeded' } });
    expect(await log?.json()).toMatchObject({ text: 'done' });
    expect(await whole?.json()).toMatchObject({ text: 'done', next: 4 });
  });
});
