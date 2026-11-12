import { describe, expect, it } from 'vitest';
import { z } from 'zod';
import type { KnownDevice } from '../webos/known-devices.ts';
import { FakeStdin } from './expo-stdin.fixtures.ts';
import { addWebOSKeys, startWebOSKeys, type ExpoTerminal } from './start-webos-keys.ts';
import { tvStartRegistry } from './tv-start-registry.ts';

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
    env: { NO_COLOR: '1' },
    dir: '/jobs',
    finder: { knownDevices: async () => [], scan: async () => [] },
    registry: tvStartRegistry({}),
    ...overrides,
    text: () => text,
  };
}

const settle = () => new Promise((resolve) => setImmediate(resolve));

async function readingKeys(stdin: FakeStdin): Promise<void> {
  stdin.startExpo();
  await settle();
}

describe('startWebOSKeys', () => {
  it('stays out of a non-interactive expo start', () => {
    const ci = new FakeStdin();
    startWebOSKeys(terminal(ci, { env: { CI: '1' } }));

    const piped = new FakeStdin();
    piped.isTTY = false;
    startWebOSKeys(terminal(piped));

    expect(ci.listenerCount('newListener')).toBe(0);
    expect(piped.listenerCount('newListener')).toBe(0);
  });

  it('adds the keys to an interactive expo start, once', () => {
    const first = new FakeStdin();
    const second = new FakeStdin();

    startWebOSKeys(terminal(first));
    startWebOSKeys(terminal(second));

    expect(first.listenerCount('newListener')).toBe(1);
    expect(second.listenerCount('newListener')).toBe(0);
  });
});

describe('addWebOSKeys', () => {
  it('registers what it can open, for a package that offers one key for every TV platform', async () => {
    const stdin = new FakeStdin();
    const shown = terminal(stdin);
    addWebOSKeys(shown);
    const provider = z
      .object({
        platform: z.string(),
        package: z.string(),
        targets: z.custom<() => Promise<unknown>>((value) => typeof value === 'function'),
        open: z.custom<(target: string | null) => Promise<void>>(
          (value) => typeof value === 'function',
        ),
      })
      .parse(shown.registry?.providers.get('webos'));

    expect(provider.platform).toBe('webos');
    expect(provider.package).toBe('@kromatv/react-native-webos');
    expect(await provider.targets()).toEqual([{ target: 'simulator', label: 'webOS simulator' }]);
    await provider.open(null);
    expect(shown.text()).toContain('No TV in Developer Mode answered');
  });

  it('adds no key when the keys are claimed, and still opens what it is asked to', async () => {
    const stdin = new FakeStdin();
    const registry = tvStartRegistry({});
    const never = {
      timeoutMs: 0,
      intervalMs: 0,
      status: async () => null,
      now: () => 0,
      sleep: async () => undefined,
    };
    const shown = terminal(stdin, { registry, poll: never });
    addWebOSKeys(shown);
    registry.claimed = true;
    await readingKeys(stdin);

    stdin.press('t');
    await settle();
    expect(shown.text()).toBe('');

    const provider = z
      .object({
        open: z.custom<(target: string | null) => Promise<void>>(
          (value) => typeof value === 'function',
        ),
      })
      .parse(registry.providers.get('webos'));
    await provider.open('10.0.0.7');
    expect(shown.text()).toBe(
      '› The dev server did not answer, so the app was not opened on webOS. Press t again once it runs.\n',
    );
  });

  it('prints its usage line, and says what to do when t finds no TV', async () => {
    const stdin = new FakeStdin();
    const shown = terminal(stdin);
    addWebOSKeys(shown);
    await readingKeys(stdin);

    stdin.press('t');

    await expect.poll(shown.text).toContain('No TV in Developer Mode answered');
    expect(shown.text()).toContain('› Press t │ open on webOS TV\n');
    expect(shown.text()).toContain('› Looking for LG TVs on the network...\n');
  });

  it('lists the targets on shift+t, and opens nothing when cancelled', async () => {
    const stdin = new FakeStdin();
    const shown = terminal(stdin);
    addWebOSKeys(shown);
    await readingKeys(stdin);

    stdin.press('T');
    await expect.poll(shown.text).toContain('  1  webOS simulator\n');
    stdin.press('\u001b');
    await settle();

    expect(shown.text()).not.toContain('Opening');
    expect(stdin.listeners('data')).toContain(stdin.handleKeypress);
  });

  it('says it is busy when t comes again before the last one finished', async () => {
    let answer: (devices: KnownDevice[]) => void = () => undefined;
    const stdin = new FakeStdin();
    const shown = terminal(stdin, {
      finder: {
        knownDevices: () =>
          new Promise((resolve) => {
            answer = resolve;
          }),
        scan: async () => [],
      },
    });
    addWebOSKeys(shown);
    await readingKeys(stdin);

    stdin.press('t');
    stdin.press('t');
    answer([]);

    await expect.poll(shown.text).toContain('No TV in Developer Mode answered');
    expect(shown.text()).toContain('› Already opening on webOS: wait for it to finish\n');
  });
});
