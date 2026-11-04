import { setTimeout as sleep } from 'node:timers/promises';
import { z } from 'zod';
import { fetchText } from '../discovery/http-text.ts';
import { FixableError } from '../errors.ts';

const RUNNING = 'packager-status:running';
const originSchema = z.url({ protocol: /^https?$/ });

/** How {@link waitForDevServer} polls, replaced in tests. */
export interface DevServerPoll {
  readonly timeoutMs: number;
  readonly intervalMs: number;
  readonly status: (url: string) => Promise<string | null>;
  readonly now: () => number;
  readonly sleep: (ms: number) => Promise<unknown>;
}

const defaultPoll: DevServerPoll = {
  timeoutMs: 120_000,
  intervalMs: 250,
  status: (url) => fetchText(url, RUNNING.length),
  now: Date.now,
  sleep,
};

async function answers(env: NodeJS.ProcessEnv, poll: DevServerPoll): Promise<string | null> {
  const origin = originSchema.safeParse(env.EXPO_DEV_SERVER_ORIGIN);
  if (!origin.success) return null;
  const status = await poll.status(new URL('/status', origin.data).toString());
  return status === RUNNING ? origin.data : null;
}

/**
 * The origin of the dev server of the `expo start` this runs in, once it
 * answers `/status`. Expo names it in `EXPO_DEV_SERVER_ORIGIN` after Metro starts.
 */
export async function waitForDevServer(
  env: NodeJS.ProcessEnv,
  poll: DevServerPoll = defaultPoll,
): Promise<string> {
  return pollUntil(env, poll, poll.now() + poll.timeoutMs);
}

async function pollUntil(
  env: NodeJS.ProcessEnv,
  poll: DevServerPoll,
  deadline: number,
): Promise<string> {
  if (poll.now() >= deadline) {
    throw new FixableError(
      'The dev server did not answer, so the app was not opened on Tizen',
      'Press t again once it runs.',
    );
  }
  const origin = await answers(env, poll);
  if (origin !== null) return origin;
  await poll.sleep(poll.intervalMs);
  return pollUntil(env, poll, deadline);
}
