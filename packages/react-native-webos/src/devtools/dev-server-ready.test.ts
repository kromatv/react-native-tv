import { createServer } from 'node:http';
import { describe, expect, it, vi } from 'vitest';
import { waitForDevServer, type DevServerPoll } from './dev-server-ready.ts';

function poll(statuses: readonly (string | null)[], onSleep: () => void = () => undefined) {
  let clock = 0;
  const status = vi.fn(
    async () => statuses[Math.min(status.mock.calls.length - 1, statuses.length - 1)] ?? null,
  );
  const fake: DevServerPoll = {
    timeoutMs: 1000,
    intervalMs: 250,
    status,
    now: () => clock,
    sleep: async (ms) => {
      clock += ms;
      onSleep();
    },
  };
  return fake;
}

describe('waitForDevServer', () => {
  it('waits for Expo to name its dev server, then for it to answer /status', async () => {
    const env: NodeJS.ProcessEnv = {};
    const fake = poll([null, 'packager-status:running'], () => {
      env.EXPO_DEV_SERVER_ORIGIN = 'http://localhost:8191';
    });

    expect(await waitForDevServer(env, fake)).toBe('http://localhost:8191');
    expect(fake.status).toHaveBeenCalledWith('http://localhost:8191/status');
    expect(fake.status).toHaveBeenCalledTimes(2);
  });

  it('gives up, saying what to press instead, when it never answers', async () => {
    const env = { EXPO_DEV_SERVER_ORIGIN: 'http://localhost:8191' };

    await expect(waitForDevServer(env, poll(['starting']))).rejects.toThrow(
      'The dev server did not answer',
    );
    await expect(waitForDevServer({ EXPO_DEV_SERVER_ORIGIN: 'ftp://x' }, poll([]))).rejects.toThrow(
      'did not answer',
    );
  });

  it("asks the dev server's /status over HTTP by default", async () => {
    const server = createServer((request, response) => {
      response.end(request.url === '/status' ? 'packager-status:running' : 'no');
    });
    await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
    const address = server.address();
    try {
      if (address === null || typeof address === 'string') throw new Error('no TCP address');
      const origin = `http://127.0.0.1:${address.port}`;

      expect(await waitForDevServer({ EXPO_DEV_SERVER_ORIGIN: origin })).toBe(origin);
    } finally {
      server.close();
    }
  });
});
