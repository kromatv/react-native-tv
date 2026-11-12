import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import {
  EXPO_METRO_OVERRIDE,
  TV_METRO_CONFIG,
  TV_PROJECT_ROOT,
} from '../metro/expo-metro-override.ts';
import withReactNativeTV from './config-plugin.ts';

let app: TestApp;

const WITH_PLUGINS = `exports.withPlugins = (config, plugins) => ({
  ...config,
  applied: [...(config.applied ?? []), ...plugins],
});`;

beforeEach(() => {
  app = testApp();
  app.dependOn('tizen', 'webos');
  app.platformPackage('tizen', { platform: 'tizen' });
  app.platformPackage('webos', { platform: 'webos' });
  app.write('node_modules/expo/package.json', {
    name: 'expo',
    exports: { './config-plugins': './config-plugins.js' },
  });
  app.write('node_modules/expo/config-plugins.js', WITH_PLUGINS);
});

afterEach(() => {
  vi.restoreAllMocks();
  delete process.env[EXPO_METRO_OVERRIDE];
  delete process.env[TV_PROJECT_ROOT];
  app.remove();
});

describe('withReactNativeTV', () => {
  it("applies each platform's plugin and adds its entry to the config, after the app's own", () => {
    const config = {
      name: 'App',
      plugins: ['expo-font'],
      _internal: { projectRoot: app.root },
    };
    const platforms = [
      ['tizen', { package: 'tv.example.app', preview: true }],
      ['webos', { package: 'tv.example.app' }],
    ];

    expect(
      withReactNativeTV(config, { package: 'tv.example.app', tizen: { preview: true } }),
    ).toEqual({ ...config, applied: platforms, plugins: ['expo-font', ...platforms] });
  });

  it('gives the Expo CLI that evaluates it a Metro config with every TV platform', () => {
    withReactNativeTV({ _internal: { projectRoot: app.root } });

    expect(process.env[EXPO_METRO_OVERRIDE]).toBe(TV_METRO_CONFIG);
    expect(process.env[TV_PROJECT_ROOT]).toBe(app.root);
  });

  it("applies react-native-tvos' plugin without listing it", () => {
    app.write('node_modules/@react-native-tvos/config-tv/package.json', {});
    const config = { _internal: { projectRoot: app.root } };

    expect(withReactNativeTV(config, { tvos: { androidTVRequired: true } })).toEqual({
      ...config,
      applied: [['@react-native-tvos/config-tv', { isTV: true, androidTVRequired: true }]],
      plugins: [],
    });
  });

  it('returns the config as it is when the props stand for no plugin', () => {
    const config = { _internal: { projectRoot: app.root } };

    expect(withReactNativeTV(config)).toBe(config);
  });

  it('reads the app in the current directory when Expo names no project root', () => {
    vi.spyOn(process, 'cwd').mockReturnValue(app.root);

    expect(withReactNativeTV({}, { webos: { package: 'tv.example.lg' } })).toEqual({
      applied: [['webos', { package: 'tv.example.lg' }]],
      plugins: [['webos', { package: 'tv.example.lg' }]],
    });
  });

  it('refuses props it cannot use', () => {
    const config = { _internal: { projectRoot: app.root } };

    expect(() => withReactNativeTV(config, { roku: {} })).toThrow(
      'Invalid @kromatv/react-native-tv props: "roku" is no TV platform this app depends on. It has tizen, tvos, webos',
    );
  });
});
