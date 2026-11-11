import { describe, expect, it, vi } from 'vitest';
import { createPublishQueue, type PublishResult } from './publish-queue.ts';

const PUBLISHED: PublishResult = { published: true, channel: 'ContentDBAPI' };

function fakeClock() {
  let now = 0;
  const timers: { at: number; run: () => void }[] = [];
  return {
    now: () => now,
    setTimeout: (run: () => void, ms: number) => timers.push({ at: now + ms, run }),
    advance(ms: number) {
      now += ms;
      for (const timer of timers.filter((entry) => entry.at <= now)) {
        timers.splice(timers.indexOf(timer), 1);
        timer.run();
      }
    },
  };
}

describe('createPublishQueue', () => {
  it('publishes the first call at once', async () => {
    const publish = vi.fn(async () => PUBLISHED);
    const submit = createPublishQueue(publish, fakeClock(), 1000);

    const result = await submit('a');

    expect(publish).toHaveBeenCalledWith('a');
    expect(result).toEqual(PUBLISHED);
  });

  it('holds a call inside the interval until the interval ends', async () => {
    const clock = fakeClock();
    const publish = vi.fn(async () => PUBLISHED);
    const submit = createPublishQueue(publish, clock, 1000);
    await submit('a');

    const second = submit('b');
    clock.advance(999);

    expect(publish).toHaveBeenCalledTimes(1);

    clock.advance(1);

    await expect(second).resolves.toEqual(PUBLISHED);
    expect(publish).toHaveBeenLastCalledWith('b');
  });

  it('publishes only the last of the calls that waited together', async () => {
    const clock = fakeClock();
    const publish = vi.fn(async () => PUBLISHED);
    const submit = createPublishQueue(publish, clock, 1000);
    await submit('a');

    const waiting = [submit('b'), submit('c')];
    clock.advance(1000);

    await expect(Promise.all(waiting)).resolves.toEqual([PUBLISHED, PUBLISHED]);
    expect(publish.mock.calls).toEqual([['a'], ['c']]);
  });

  it('resolves with the failure when the publish throws', async () => {
    const submit = createPublishQueue(async () => {
      throw new Error('no Smart Hub');
    }, fakeClock());

    await expect(submit('a')).resolves.toEqual({ published: false, reason: 'Error: no Smart Hub' });
  });

  it("waits out Samsung's interval on the system clock by default", async () => {
    vi.useFakeTimers();
    const publish = vi.fn(async () => PUBLISHED);
    const submit = createPublishQueue(publish);
    await submit('a');

    const second = submit('b');
    await vi.advanceTimersByTimeAsync(10 * 60 * 1000);

    await expect(second).resolves.toEqual(PUBLISHED);
    expect(publish).toHaveBeenLastCalledWith('b');
    vi.useRealTimers();
  });
});
