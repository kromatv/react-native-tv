import { spawn } from 'node:child_process';
import { open } from 'node:fs/promises';
import { extname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { logPath, readJob, writeJob } from './jobs.ts';

const CLI = fileURLToPath(new URL(`../cli/main${extname(import.meta.url)}`, import.meta.url));

async function runJob(dir: string, id: string, cli: string): Promise<void> {
  const job = await readJob(dir, id);
  if (job === null) return;
  const log = await open(logPath(dir, id), 'a');
  await writeJob(dir, { ...job, pid: process.pid });
  const child = spawn(process.execPath, [cli, ...job.args], {
    cwd: job.cwd,
    stdio: ['ignore', log.fd, log.fd],
    env: { ...process.env, CI: '1' },
  });
  const exitCode = await new Promise<number | null>((resolve) => {
    child.on('error', () => resolve(null));
    child.on('close', resolve);
  });
  await log.close();
  const latest = (await readJob(dir, id)) ?? job;
  if (latest.status !== 'running') return;
  await writeJob(dir, {
    ...latest,
    status: exitCode === 0 ? 'succeeded' : 'failed',
    exitCode,
    endedAt: new Date().toISOString(),
  });
}

/** `node job-runner <dir> <id>`: runs `rn-tizen` with the job's arguments, its output in the job's log. */
export async function runJobFromArgv(argv: readonly string[], cli = CLI): Promise<void> {
  const [dir, id] = argv;
  if (dir !== undefined && id !== undefined) await runJob(dir, id, cli);
}
