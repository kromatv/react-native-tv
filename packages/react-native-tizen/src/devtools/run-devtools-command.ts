import { z } from 'zod';
import { CommandInputError, jobPageUrl, planCommand } from './devtools-commands.ts';
import { jobsDir, listJobs, startJob, stopJob } from './jobs.ts';

/** One line of the output Expo shows for a dev tools command. */
export interface OutputLine {
  readonly type: 'text';
  readonly text: string;
  readonly level: 'info' | 'warning' | 'error';
  readonly url?: string;
}

const argvSchema = z.tuple([z.string().min(1), z.string().max(8192), z.url()]);

async function start(argv: readonly string[], dir: string): Promise<OutputLine> {
  const [command, argsJson, devServer] = argvSchema.parse(argv);
  const plan = await planCommand(command, JSON.parse(argsJson), devServer);
  if (plan.exclusive) {
    const running = (await listJobs(dir)).filter(
      (job) => job.kind === plan.kind && job.status === 'running',
    );
    await Promise.all(running.map((job) => stopJob(dir, job.id)));
  }
  const job = await startJob(dir, {
    kind: plan.kind,
    title: plan.title,
    args: [...plan.args],
    cwd: process.cwd(),
  });
  const url = jobPageUrl(devServer, job.id);
  return { type: 'text', level: 'info', text: `${plan.title} started; progress and logs`, url };
}

function failure(error: unknown): string {
  if (error instanceof CommandInputError || error instanceof z.ZodError) return error.message;
  const reason = error instanceof Error ? error.message : String(error);
  return `rn-tizen could not start the command: ${reason}`;
}

/**
 * What Expo runs for a command under shift+m: `node <entry> <command> <args>
 * <dev server>`. Expo kills it after ten seconds, so it starts the work as a
 * detached job and answers with where to follow it.
 */
export async function runDevtoolsCommand(
  argv: readonly string[],
  dir = jobsDir(),
): Promise<{ lines: OutputLine[]; exitCode: number }> {
  try {
    return { lines: [await start(argv, dir)], exitCode: 0 };
  } catch (error) {
    return { lines: [{ type: 'text', level: 'error', text: failure(error) }], exitCode: 1 };
  }
}
