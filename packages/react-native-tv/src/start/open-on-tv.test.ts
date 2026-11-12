import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { lastTvTarget, rememberTvTarget } from './last-tv-target.ts';
import { chooseTvTarget, openOnLastTv, type TvOpenIo } from './open-on-tv.ts';
import { fakeProvider, type FakeProvider } from './provider.fixtures.ts';
import { tvStartRegistry } from './tv-start-registry.ts';

let dir: string;
let printed: string;
let asked: (readonly string[])[];
let tizen: FakeProvider;
let webos: FakeProvider;

function io(providers: readonly FakeProvider[], answer: number | null = null): TvOpenIo {
  const registry = tvStartRegistry({});
  for (const provider of providers) registry.providers.set(provider.platform, provider);
  return {
    write: (text) => {
      printed += text;
    },
    registry,
    projectRoot: '/app',
    lastTargetFile: join(dir, 'last-target.json'),
    ask: (labels) => {
      asked.push(labels);
      return Promise.resolve(answer);
    },
  };
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tv-open-'));
  printed = '';
  asked = [];
  tizen = fakeProvider('tizen', [
    { target: '10.0.0.7', label: '10.0.0.7  (used last)' },
    { target: 'simulator', label: 'Tizen simulator' },
  ]);
  webos = fakeProvider('webos', [{ target: 'simulator', label: 'webOS simulator' }]);
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('chooseTvTarget', () => {
  it('lists the targets of every platform, opens the one chosen and remembers it', async () => {
    const given = io([webos, tizen], 2);
    await chooseTvTarget(given);

    expect(asked).toEqual([
      ['tizen  10.0.0.7  (used last)', 'tizen  Tizen simulator', 'webos  webOS simulator'],
    ]);
    expect(webos.opened).toEqual(['simulator']);
    expect(tizen.opened).toEqual([]);
    expect(lastTvTarget(given.lastTargetFile, '/app')).toEqual({
      platform: 'webos',
      target: 'simulator',
    });
  });

  it('opens nothing and remembers nothing when cancelled', async () => {
    const given = io([tizen, webos], null);
    await chooseTvTarget(given);

    expect([tizen.opened, webos.opened]).toEqual([[], []]);
    expect(lastTvTarget(given.lastTargetFile, '/app')).toBeNull();
  });

  it('says so when no platform has registered, or none has a target', async () => {
    await chooseTvTarget(io([]));
    await chooseTvTarget(io([fakeProvider('roku', [])]));

    expect(printed).toBe(
      '› No TV platform package of this app offers a target here: each needs a version that registers with expo start\n' +
        '› No TV or simulator to open on\n',
    );
    expect(asked).toEqual([]);
  });

  it('says which package failed to open, without throwing', async () => {
    const failing = {
      ...webos,
      open: () => Promise.reject(new Error('no route')),
    };
    await chooseTvTarget(io([failing], 0));

    expect(printed).toBe('› @tv/webos could not open the app: no route\n');
  });
});

describe('openOnLastTv', () => {
  it('opens where the project opened last', async () => {
    const given = io([tizen, webos]);
    rememberTvTarget(given.lastTargetFile, '/app', { platform: 'tizen', target: '10.0.0.7' });
    await openOnLastTv(given);

    expect(tizen.opened).toEqual(['10.0.0.7']);
    expect(asked).toEqual([]);
  });

  it("opens on the TV used last of an app's one platform when nothing is remembered", async () => {
    await openOnLastTv(io([webos]));

    expect(webos.opened).toEqual([null]);
    expect(asked).toEqual([]);
  });

  it('asks in an app with several platforms and nothing remembered, or a platform that is gone', async () => {
    await openOnLastTv(io([tizen, webos], 0));
    const given = io([tizen, webos], 2);
    rememberTvTarget(given.lastTargetFile, '/app', { platform: 'roku', target: 'tv' });
    await openOnLastTv(given);

    expect(asked).toHaveLength(2);
    expect(tizen.opened).toEqual(['10.0.0.7']);
    expect(webos.opened).toEqual(['simulator']);
  });

  it('says so when no platform has registered', async () => {
    await openOnLastTv(io([]));

    expect(printed).toContain('No TV platform package of this app offers a target here');
  });
});
