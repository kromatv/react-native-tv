/** How a publish ended. */
export type PublishResult =
  | { readonly published: true; readonly channel: string }
  | { readonly published: false; readonly reason: string };

/** Samsung takes one personal preview update per 10 minutes. */
export const MIN_PUBLISH_INTERVAL_MS = 10 * 60 * 1000;

interface Clock {
  now(): number;
  setTimeout(run: () => void, ms: number): unknown;
}

const SYSTEM_CLOCK: Clock = {
  now: () => Date.now(),
  setTimeout: (run, ms) => setTimeout(run, ms),
};

interface Waiter {
  resolve(result: PublishResult): void;
}

/**
 * Publishes at most once per `intervalMs`: a call inside the interval waits for
 * its end, and calls that wait together publish the last JSON only, each
 * resolving with that publish's result.
 */
export function createPublishQueue(
  publish: (json: string) => Promise<PublishResult>,
  clock: Clock = SYSTEM_CLOCK,
  intervalMs = MIN_PUBLISH_INTERVAL_MS,
): (json: string) => Promise<PublishResult> {
  let lastPublishAt: number | null = null;
  let pending: { json: string; waiters: Waiter[] } | null = null;

  const flush = () => {
    const batch = pending;
    pending = null;
    if (batch === null) return;
    lastPublishAt = clock.now();
    void publish(batch.json)
      .catch((error: unknown): PublishResult => ({ published: false, reason: String(error) }))
      .then((result) => {
        for (const waiter of batch.waiters) waiter.resolve(result);
      });
  };

  return (json) =>
    new Promise<PublishResult>((resolve) => {
      if (pending !== null) {
        pending = { json, waiters: [...pending.waiters, { resolve }] };
        return;
      }
      pending = { json, waiters: [{ resolve }] };
      const wait = lastPublishAt === null ? 0 : lastPublishAt + intervalMs - clock.now();
      if (wait <= 0) flush();
      else clock.setTimeout(flush, wait);
    });
}
