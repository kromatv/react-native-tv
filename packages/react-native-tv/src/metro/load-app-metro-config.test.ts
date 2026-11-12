import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { loadAppMetroConfig } from './load-app-metro-config.ts';

let app: TestApp;

const ADDS = (platform: string) =>
  `exports.withTvPlatform = (config) => ({ ...config, resolver: { ...config.resolver, platforms: [...config.resolver.platforms, '${platform}'] } });`;

function installExpo(platforms: readonly string[]): void {
  app.write('node_modules/expo/package.json', {
    name: 'expo',
    exports: { './metro-config': './metro-config.js', './package.json': './package.json' },
  });
  app.write(
    'node_modules/expo/metro-config.js',
    `exports.loadUserConfig = async (params) => ({ loadedWith: params, resolver: { platforms: ${JSON.stringify(platforms)} } });`,
  );
  app.write('node_modules/@expo/config/package.json', {
    name: '@expo/config',
    exports: { './paths': './paths.js' },
  });
  app.write(
    'node_modules/@expo/config/paths.js',
    'exports.getMetroServerRoot = (root) => root + "/..";',
  );
}

beforeEach(() => {
  app = testApp();
  app.dependOn('tizen', 'webos', 'unnamed');
  app.platformPackage('tizen', { platform: 'tizen' }, ADDS('tizen'));
  app.platformPackage('webos', { platform: 'webos' }, ADDS('webos'));
  app.platformPackage('unnamed', {}, ADDS('unnamed'));
});

afterEach(() => {
  app.remove();
});

describe('loadAppMetroConfig', () => {
  it("loads the app's Metro config as Expo does, and adds the platforms that name themselves", async () => {
    installExpo(['ios', 'android']);

    expect(await loadAppMetroConfig(app.root)).toEqual({
      loadedWith: { projectRoot: app.root, serverRoot: `${app.root}/..` },
      projectRoot: app.root,
      resolver: { platforms: ['ios', 'android', 'tizen', 'webos'] },
    });
  });

  it("leaves a platform the app's own config added already", async () => {
    installExpo(['ios', 'android', 'webos']);
    const config = await loadAppMetroConfig(app.root);

    expect(config.resolver).toEqual({ platforms: ['ios', 'android', 'webos', 'tizen'] });
  });

  it('fails in an app without Expo', async () => {
    await expect(loadAppMetroConfig(app.root)).rejects.toThrow('expo/package.json');
  });
});
