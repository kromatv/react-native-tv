import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { CommandInputError, jobPageUrl, planCommand } from './devtools-commands.ts';
import { jobsDir } from './jobs.ts';
import { startPlannedJob } from './start-planned-job.ts';

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
  const job = await startPlannedJob(dir, plan, process.cwd());
  const url = jobPageUrl(devServer, job.id);
  return { type: 'text', level: 'info', text: `${plan.title} started; progress and logs`, url };
}

/** What a dev tools command that could not run tells the user. */
export function describeFailure(error: unknown): string {
  if (error instanceof CommandInputError || error instanceof z.ZodError) return error.message;
  if (error instanceof FixableError) return `${error.message}. ${error.hint}`;
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
    return { lines: [{ type: 'text', level: 'error', text: describeFailure(error) }], exitCode: 1 };
  }
}
