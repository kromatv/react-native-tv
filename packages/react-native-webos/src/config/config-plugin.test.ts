import { describe, expect, it } from 'vitest';
import withReactNativeWebOS from './config-plugin.ts';

describe('withReactNativeWebOS', () => {
  it('returns the config unchanged, with or without props', () => {
    const config = { name: 'My App' };

    expect(withReactNativeWebOS(config)).toBe(config);
    expect(withReactNativeWebOS(config, { package: 'org.example.app', title: 'TV' })).toBe(config);
  });

  it('throws every bad prop at once, named after the plugin', () => {
    expect(() => withReactNativeWebOS({}, { package: 'My_App', version: '1.0' })).toThrow(
      /package:[\s\S]*version:/,
    );
  });
});
