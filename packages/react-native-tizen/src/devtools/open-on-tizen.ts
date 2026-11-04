import { waitForDevServer, type DevServerPoll } from './dev-server-ready.ts';
import { openPlan } from './devtools-commands.ts';
import { followJob, type JobFollower } from './follow-job.ts';
import { describeFailure } from './run-devtools-command.ts';
import { startPlannedJob } from './start-planned-job.ts';
import { SIMULATOR } from './tizen-targets.ts';

/** What {@link openOnTizen} talks to, replaced in tests. */
export interface OpenIo {
  readonly write: (text: string) => void;
  readonly dir: string;
  readonly poll?: DevServerPoll;
  readonly follower?: Partial<JobFollower>;
}

/** Writes one line in the style of Expo's own messages. */
export function say(write: (text: string) => void, line: string): void {
  write(`› ${line}\n`);
}

/**
 * Opens the app where `choose` says, against the dev server of the `expo
 * start` this runs in, as `a` opens it on Android: prints the run's progress,
 * and a line saying what went wrong instead of throwing.
 */
export async function openOnTizen(
  choose: () => Promise<string | null>,
  env: NodeJS.ProcessEnv,
  io: OpenIo,
): Promise<void> {
  try {
    const target = await choose();
    if (target === null) return;
    const devServer = await waitForDevServer(env, io.poll);
    const simulator = target === SIMULATOR;
    say(io.write, simulator ? 'Opening in the Tizen simulator...' : `Opening on TV ${target}...`);
    const plan = openPlan(target, devServer);
    const job = await startPlannedJob(io.dir, plan, env.EXPO_PUBLIC_PROJECT_ROOT ?? process.cwd());
    const ended = await followJob(io.dir, job.id, { ...io.follower, write: io.write });
    if (ended?.status !== 'succeeded') {
      say(io.write, `${plan.title} did not finish: fix the step above, then press t again`);
    }
  } catch (error) {
    say(io.write, describeFailure(error));
  }
}
