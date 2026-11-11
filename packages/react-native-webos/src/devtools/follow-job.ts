import { setTimeout as sleep } from 'node:timers/promises';
import { readJob, readLog, type Job } from './jobs.ts';

/** How {@link followJob} reads and waits, replaced in tests. */
export interface JobFollower {
  readonly write: (text: string) => void;
  readonly intervalMs: number;
  readonly sleep: (ms: number) => Promise<unknown>;
  /** Runs `stop` if this process exits first; the returned function cancels that. */
  readonly onExit: (stop: () => void) => () => void;
}

const defaultFollower: Omit<JobFollower, 'write'> = {
  intervalMs: 250,
  sleep,
  onExit: (stop) => {
    process.once('exit', stop);
    return () => process.off('exit', stop);
  },
};

function stopNow(job: Job | null): void {
  if (job?.status !== 'running' || job.pid === undefined) return;
  try {
    process.kill(-job.pid, 'SIGTERM');
  } catch {
    return;
  }
}

async function copyUntilEnd(
  dir: string,
  id: string,
  from: number,
  follower: JobFollower,
  seen: (job: Job | null) => void,
): Promise<Job | null> {
  const log = await readLog(dir, id, from);
  if (log.text !== '') follower.write(log.text);
  const job = await readJob(dir, id);
  seen(job);
  if (job?.status !== 'running' && log.text === '') return job;
  await follower.sleep(follower.intervalMs);
  return copyUntilEnd(dir, id, log.next, follower, seen);
}

/**
 * Writes what a job prints as it prints it, until it ends, and returns how it
 * ended. The job stops if this process exits first.
 */
export async function followJob(
  dir: string,
  id: string,
  follower: Pick<JobFollower, 'write'> & Partial<JobFollower>,
): Promise<Job | null> {
  const settings: JobFollower = { ...defaultFollower, ...follower };
  let job = await readJob(dir, id);
  const cancel = settings.onExit(() => stopNow(job));
  try {
    return await copyUntilEnd(dir, id, 0, settings, (latest) => {
      job = latest;
    });
  } finally {
    cancel();
  }
}
