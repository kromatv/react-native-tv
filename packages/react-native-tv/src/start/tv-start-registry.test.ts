import { describe, expect, it } from 'vitest';
import { fakeProvider } from './provider.fixtures.ts';
import {
  providerTargets,
  TV_START_REGISTRY,
  tvStartProviders,
  tvStartRegistry,
} from './tv-start-registry.ts';

describe('tvStartRegistry', () => {
  it('makes the registry once, unclaimed and empty, under a key every package can name', () => {
    const host = {};
    const registry = tvStartRegistry(host);

    expect(registry).toEqual({ claimed: false, providers: new Map() });
    expect(tvStartRegistry(host)).toBe(registry);
    expect(Reflect.get(host, Symbol.for('reactNativeTvPlatform.start'))).toBe(registry);
    expect(TV_START_REGISTRY.description).toBe('reactNativeTvPlatform.start');
  });

  it('replaces a value left there in another shape', () => {
    const host = { [TV_START_REGISTRY]: { claimed: 'yes', providers: [] } };

    expect(tvStartRegistry(host)).toEqual({ claimed: false, providers: new Map() });
  });

  it('is on globalThis by default', () => {
    expect(tvStartRegistry()).toBe(Reflect.get(globalThis, TV_START_REGISTRY));
  });
});

describe('tvStartProviders', () => {
  it('gives the providers in the order of their platforms, without those in another shape', () => {
    const registry = tvStartRegistry({});
    const webos = fakeProvider('webos', []);
    const tizen = fakeProvider('tizen', []);
    registry.providers.set('webos', webos);
    registry.providers.set('old', { platform: 'old', targets: 'none' });
    registry.providers.set('tizen', tizen);

    expect(tvStartProviders(registry).map(({ platform }) => platform)).toEqual(['tizen', 'webos']);
  });
});

describe('providerTargets', () => {
  it("gives a provider's targets", async () => {
    const targets = [{ target: '10.0.0.7', label: 'living room' }];

    expect(await providerTargets(fakeProvider('webos', targets))).toEqual(targets);
  });

  it('gives none for a provider that fails or answers in another shape', async () => {
    const failing = { ...fakeProvider('webos', []), targets: () => Promise.reject(new Error('x')) };
    const odd = { ...fakeProvider('webos', []), targets: () => Promise.resolve([{ target: 1 }]) };

    expect(await providerTargets(failing)).toEqual([]);
    expect(await providerTargets(Object(odd))).toEqual([]);
  });
});
