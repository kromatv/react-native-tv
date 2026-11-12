import { afterEach, describe, expect, it, vi } from 'vitest';
import { FixableError } from '../errors.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { simulatorInstance } from './instance.ts';
import { openInBrowser, startViewer, viewerUrl } from './viewer.ts';

vi.mock('./simulator.ts', () => ({ runInstanceScript: vi.fn() }));

const instance = simulatorInstance('ts');

afterEach(() => {
  vi.unstubAllGlobals();
  vi.useRealTimers();
});

describe('startViewer', () => {
  it("returns the live screen's URL once the forward answers", async () => {
    const fetch = vi
      .fn()
      .mockRejectedValueOnce(new Error('refused'))
      .mockResolvedValueOnce({ ok: true });
    vi.stubGlobal('fetch', fetch);

    expect(await startViewer(recordingTask(), instance)).toBe(viewerUrl(instance));
    expect(fetch).toHaveBeenCalledTimes(2);
  });

  it('says what to check when the forward never answers', async () => {
    vi.useFakeTimers();
    vi.stubGlobal(
      'fetch',
      vi.fn(async () => ({ ok: false })),
    );

    const starting = startViewer(recordingTask(), instance);
    const failed = expect(starting).rejects.toBeInstanceOf(FixableError);
    await vi.advanceTimersByTimeAsync(16_000);

    await failed;
  });
});

describe('openInBrowser', () => {
  it("opens the URL with macOS's open", async () => {
    const task = recordingTask();

    await openInBrowser(task, 'http://127.0.0.1:6080/');

    expect(task.commands).toEqual([['/usr/bin/open', 'http://127.0.0.1:6080/']]);
  });
});
