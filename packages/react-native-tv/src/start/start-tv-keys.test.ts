import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FakeStdin } from './expo-stdin.fixtures.ts';
import { fakeProvider } from './provider.fixtures.ts';
import { addTvKeys, startTvKeys, type ExpoTerminal } from './start-tv-keys.ts';
import { tvStartRegistry } from './tv-start-registry.ts';

let dir: string;

function terminal(
  stdin: FakeStdin,
  overrides: Partial<ExpoTerminal> = {},
): ExpoTerminal & { text: () => string } {
  let text = '';
  return {
    input: stdin,
    output: {
      isTTY: true,
      write: (chunk: string) => {
        text += chunk;
      },
    },
    env: { NO_COLOR: '1', RN_TV_CACHE_DIR: dir },
    registry: tvStartRegistry({}),
    cwd: '/app',
    ...overrides,
    text: () => text,
  };
}

const settle = () => new Promise((resolve) => setImmediate(resolve));

async function readingKeys(stdin: FakeStdin): Promise<void> {
  stdin.startExpo();
  await settle();
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tv-keys-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('startTvKeys', () => {
  it('stays out of a non-interactive expo start, and claims nothing there', () => {
    const ci = new FakeStdin();
    const inCi = terminal(ci, { env: { CI: '1' } });
    startTvKeys(inCi);

    const piped = new FakeStdin();
    piped.isTTY = false;
    startTvKeys(terminal(piped));

    expect(ci.listenerCount('newListener')).toBe(0);
    expect(piped.listenerCount('newListener')).toBe(0);
    expect(inCi.registry.claimed).toBe(false);
  });

  it('adds the keys to an interactive expo start, once', () => {
    const first = new FakeStdin();
    const second = new FakeStdin();

    startTvKeys(terminal(first));
    startTvKeys(terminal(second));

    expect(first.listenerCount('newListener')).toBe(1);
    expect(second.listenerCount('newListener')).toBe(0);
  });
});

describe('addTvKeys', () => {
  it('claims the keys for every platform and prints one usage line', async () => {
    const stdin = new FakeStdin();
    const shown = terminal(stdin);
    addTvKeys(shown);
    await readingKeys(stdin);
    stdin.press('?');
    await settle();

    expect(shown.registry.claimed).toBe(true);
    expect(shown.text()).toBe(
      '› Press t │ open on TV\n› Press t │ open on TV\n› shift+t │ select a TV or a simulator\n',
    );
  });

  it('lists every platform on shift+t, opens the one picked, and t opens it again', async () => {
    const stdin = new FakeStdin();
    const shown = terminal(stdin);
    const tizen = fakeProvider('tizen', [{ target: '10.0.0.7', label: '10.0.0.7' }]);
    const webos = fakeProvider('webos', [{ target: 'simulator', label: 'webOS simulator' }]);
    shown.registry.providers.set('webos', webos);
    shown.registry.providers.set('tizen', tizen);
    addTvKeys(shown);
    await readingKeys(stdin);

    stdin.press('T');
    await expect.poll(shown.text).toContain('  2  webos  webOS simulator\n');
    stdin.press('2');
    await expect.poll(() => webos.opened).toEqual(['simulator']);
    await settle();
    stdin.press('t');
    await expect.poll(() => webos.opened).toEqual(['simulator', 'simulator']);

    expect(tizen.opened).toEqual([]);
    expect(stdin.listeners('data')).toContain(stdin.handleKeypress);
  });

  it('says it is busy when t comes again before the last one finished', async () => {
    let finish: () => void = () => undefined;
    const stdin = new FakeStdin();
    const shown = terminal(stdin, {
      env: { NO_COLOR: '1', RN_TV_CACHE_DIR: dir, EXPO_PUBLIC_PROJECT_ROOT: '/other' },
    });
    shown.registry.providers.set('webos', {
      ...fakeProvider('webos', []),
      open: () =>
        new Promise<void>((resolve) => {
          finish = resolve;
        }),
    });
    addTvKeys(shown);
    await readingKeys(stdin);

    stdin.press('t');
    await settle();
    stdin.press('t');
    finish();

    await expect.poll(shown.text).toContain('› Already opening on a TV: wait for it to finish\n');
  });
});
