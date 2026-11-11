import { describe, expect, it } from 'vitest';
import withReactNativeWebOS from './config-plugin.ts';

describe('withReactNativeWebOS', () => {
  it('returns the config unchanged, with or without props', () => {
    const config = { name: 'My App' };

    expect(withReactNativeWebOS(config)).toBe(config);
    expect(withReactNativeWebOS(config, { package: 'org.example.app', voice: true })).toBe(config);
  });

  it('throws every bad prop at once, named after the plugin', () => {
    expect(() => withReactNativeWebOS({}, { package: 'my-app', resolution: '8k' })).toThrow(
      /package:[\s\S]*resolution:/,
    );
  });
});
