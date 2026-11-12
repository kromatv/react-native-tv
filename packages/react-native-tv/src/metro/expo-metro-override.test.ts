import { describe, expect, it } from 'vitest';
import {
  EXPO_METRO_OVERRIDE,
  overrideExpoMetroConfig,
  TV_METRO_CONFIG,
  TV_PROJECT_ROOT,
} from './expo-metro-override.ts';

describe('overrideExpoMetroConfig', () => {
  it("points Expo at this package's Metro config, and says which app it is for", () => {
    const env: NodeJS.ProcessEnv = {};
    overrideExpoMetroConfig('/app', env);

    expect(env).toEqual({ [EXPO_METRO_OVERRIDE]: TV_METRO_CONFIG, [TV_PROJECT_ROOT]: '/app' });
    expect(EXPO_METRO_OVERRIDE).toBe('EXPO_OVERRIDE_METRO_CONFIG');
    expect(TV_METRO_CONFIG).toMatch(/[/\\]metro[/\\]tv-metro-config\.mjs$/);
  });

  it('follows the app Expo evaluates last', () => {
    const env: NodeJS.ProcessEnv = {};
    overrideExpoMetroConfig('/first', env);
    overrideExpoMetroConfig('/second', env);

    expect(env[TV_PROJECT_ROOT]).toBe('/second');
  });

  it('leaves a Metro config something else put there', () => {
    const env: NodeJS.ProcessEnv = { [EXPO_METRO_OVERRIDE]: './other.config.js' };
    overrideExpoMetroConfig('/app', env);

    expect(env).toEqual({ [EXPO_METRO_OVERRIDE]: './other.config.js' });
  });
});
