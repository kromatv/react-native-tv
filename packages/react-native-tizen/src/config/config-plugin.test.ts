import { describe, expect, it } from 'vitest';
import withReactNativeTizen from './config-plugin.ts';

describe('withReactNativeTizen', () => {
  it('returns the config unchanged, with or without props', () => {
    const config = { name: 'My App' };

    expect(withReactNativeTizen(config)).toBe(config);
    expect(withReactNativeTizen(config, { package: 'org.example.app', voice: true })).toBe(config);
  });

  it('throws every bad prop at once, named after the plugin', () => {
    expect(() => withReactNativeTizen({}, { package: 'my-app', resolution: '8k' })).toThrow(
      /package:[\s\S]*resolution:/,
    );
  });
});
