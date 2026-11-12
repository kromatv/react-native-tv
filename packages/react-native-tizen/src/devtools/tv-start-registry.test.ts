import { describe, expect, it } from 'vitest';
import { TV_START_REGISTRY, tvStartRegistry } from './tv-start-registry.ts';

describe('tvStartRegistry', () => {
  it('makes the registry once, unclaimed and empty, under a key every package can name', () => {
    const host = {};
    const registry = tvStartRegistry(host);

    expect(registry).toEqual({ claimed: false, providers: new Map() });
    expect(tvStartRegistry(host)).toBe(registry);
    expect(Reflect.get(host, Symbol.for('reactNativeTvPlatform.start'))).toBe(registry);
  });

  it('replaces a value left there in another shape', () => {
    const host = { [TV_START_REGISTRY]: { claimed: 'yes', providers: [] } };

    expect(tvStartRegistry(host)).toEqual({ claimed: false, providers: new Map() });
  });

  it('is on globalThis by default', () => {
    expect(tvStartRegistry()).toBe(Reflect.get(globalThis, TV_START_REGISTRY));
  });
});
