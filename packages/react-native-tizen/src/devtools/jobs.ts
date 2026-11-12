import { spawn } from 'node:child_process';
import { randomBytes } from 'node:crypto';
import { existsSync } from 'node:fs';
import { mkdir, open, readdir, readFile, rm, writeFile } from 'node:fs/promises';
import { extname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { z } from 'zod';
import { cacheDir } from '../cache-dir.ts';

/** One `rn-tizen` command the dev tools started, and how it went. */
export const jobSchema = z.object({
  id: z.string().regex(/^[a-z0-9-]{1,64}$/),
  kind: z.string().min(1).max(64),
  title: z.string().min(1).max(200),
  args: z.array(z.string().max(4096)).max(64),
  cwd: z.string().min(1),
  status: z.enum(['running', 'succeeded', 'failed', 'stopped']),
  startedAt: z.string(),
  endedAt: z.string().optional(),
  exitCode: z.number().int().nullable().optional(),
  pid: z.number().int().positive().optional(),
});
export type Job = z.infer<typeof jobSchema>;

const KEPT_JOBS = 20;
const MAX_JOB_FILE = 64 * 1024;
const MAX_LOG_CHUNK = 256 * 1024;
const RUNNER = fileURLToPath(new URL(`./job-runner${extname(import.meta.url)}`, import.meta.url));

/** Where the dev tools keep their jobs: one JSON record and one log per job. */
export function jobsDir(env: NodeJS.ProcessEnv = process.env): string {
  return join(cacheDir(env), 'devtools', 'jobs');
}

const recordPath = (dir: string, id: string) => join(dir, `${id}.json`);
export const logPath = (dir: string, id: string) => join(dir, `${id}.log`);

export async function readJob(dir: string, id: string): Promise<Job | null> {
  if (!jobSchema.shape.id.safeParse(id).success || !existsSync(recordPath(dir, id))) return null;
  const text = await readFile(recordPath(dir, id), 'utf8');
  if (text.length > MAX_JOB_FILE) return null;
  try {
    const parsed = jobSchema.safeParse(JSON.parse(text));
    return parsed.success ? parsed.data : null;
  } catch {
    return null;
  }
}

export async function writeJob(dir: string, job: Job): Promise<void> {
  await writeFile(recordPath(dir, job.id), `${JSON.stringify(job, null, 2)}\n`);
}

/** Every job, the newest first. */
export async function listJobs(dir: string): Promise<Job[]> {
  const names = existsSync(dir) ? await readdir(dir) : [];
  const ids = names.filter((name) => name.endsWith('.json')).map((name) => name.slice(0, -5));
  const jobs = await Promise.all(ids.map((id) => readJob(dir, id)));
  return jobs
    .filter((job) => job !== null)
    .sort((left, right) => right.startedAt.localeCompare(left.startedAt));
}

async function prune(dir: string): Promise<void> {
  const old = (await listJobs(dir)).slice(KEPT_JOBS).filter((job) => job.status !== 'running');
  await Promise.all(
    old.flatMap((job) => [
      rm(recordPath(dir, job.id), { force: true }),
      rm(logPath(dir, job.id), { force: true }),
    ]),
  );
}

/** What a job's log holds from byte `from` on, and where the next read starts. */
export async function readLog(
  dir: string,
  id: string,
  from: number,
): Promise<{ text: string; next: number }> {
  if (!existsSync(logPath(dir, id))) return { text: '', next: from };
  const file = await open(logPath(dir, id), 'r');
  try {
    const buffer = Buffer.alloc(MAX_LOG_CHUNK);
    const { bytesRead } = await file.read(buffer, 0, MAX_LOG_CHUNK, from);
    return { text: buffer.subarray(0, bytesRead).toString('utf8'), next: from + bytesRead };
  } finally {
    await file.close();
  }
}

/** Starts `rn-tizen <args>` in `cwd`, detached, so it outlives the process that asked for it. */
export async function startJob(
  dir: string,
  request: Pick<Job, 'kind' | 'title' | 'args' | 'cwd'>,
  runner = RUNNER,
): Promise<Job> {
  await mkdir(dir, { recursive: true });
  await prune(dir);
  const job: Job = {
    ...request,
    id: `${Date.now().toString(36)}-${randomBytes(4).toString('hex')}`,
    status: 'running',
    startedAt: new Date().toISOString(),
  };
  await writeJob(dir, job);
  const child = spawn(process.execPath, [runner, dir, job.id], {
    detached: true,
    stdio: 'ignore',
    cwd: request.cwd,
  });
  child.unref();
  return job;
}

/** Stops a running job and everything it started. */
export async function stopJob(dir: string, id: string): Promise<Job | null> {
  const job = await readJob(dir, id);
  if (job?.status !== 'running' || job.pid === undefined) return job;
  try {
    process.kill(-job.pid, 'SIGTERM');
  } catch {
    return job;
  }
  const stopped: Job = { ...job, status: 'stopped', endedAt: new Date().toISOString() };
  await writeJob(dir, stopped);
  return stopped;
}
