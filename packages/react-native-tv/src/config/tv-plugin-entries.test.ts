import { realpathSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, TV_PACKAGE_DIR, type TestApp } from '../app.fixtures.ts';
import { tvPlatformPackages } from '../platforms/tv-platform-packages.ts';
import { tvPluginEntries, TVOS_PLUGIN } from './tv-plugin-entries.ts';
import { tvProps } from './tv-props.ts';

let app: TestApp;

function entries(props: unknown, listed: string[] = []) {
  return tvPluginEntries(app.root, tvProps(props), tvPlatformPackages(app.root), new Set(listed));
}

function installTvosPlugin(): void {
  app.write(`node_modules/${TVOS_PLUGIN}/package.json`, { name: TVOS_PLUGIN });
}

beforeEach(() => {
  app = testApp();
  app.dependOn('tizen', 'webos', 'metro-only');
  app.platformPackage('tizen', { platform: 'tizen' });
  app.platformPackage('webos', { platform: 'webos' });
  app.platformPackage('metro-only', {});
});

afterEach(() => {
  app.remove();
});

describe('tvPluginEntries', () => {
  it('gives every platform the shared fields, and each its own block over them', () => {
    const props = {
      package: 'tv.example.app',
      locales: ['fr'],
      tizen: { preview: true, locales: ['en'] },
    };

    const platforms = [
      ['tizen', { package: 'tv.example.app', locales: ['en'], preview: true }],
      ['webos', { package: 'tv.example.app', locales: ['fr'] }],
    ];
    expect(entries(props)).toEqual({ platforms, tvos: null, applied: platforms });
  });

  it('has Expo apply a plugin the app cannot reach by its file, and keeps its entry by name', () => {
    app.dependOn('@kromatv/react-native-tv', 'webos');
    app.bring({ roku: { platform: 'roku' }, [TVOS_PLUGIN]: {} });
    const brought = join(realpathSync(app.root), TV_PACKAGE_DIR, 'node_modules');

    expect(entries({ package: 'tv.example.app' })).toEqual({
      platforms: [
        ['roku', { package: 'tv.example.app' }],
        ['webos', { package: 'tv.example.app' }],
      ],
      tvos: [TVOS_PLUGIN, { isTV: true }],
      applied: [
        [join(brought, 'roku/app.plugin.js'), { package: 'tv.example.app' }],
        ['webos', { package: 'tv.example.app' }],
        [join(brought, TVOS_PLUGIN, 'app.plugin.js'), { isTV: true }],
      ],
    });
  });

  it('gives a platform no entry without a package for it, and one from its block alone', () => {
    const platforms = [['webos', { locales: ['fr'], package: 'tv.example.lg' }]];
    expect(entries({ locales: ['fr'], webos: { package: 'tv.example.lg' } })).toEqual({
      platforms,
      tvos: null,
      applied: platforms,
    });
    expect(entries(undefined)).toEqual({ platforms: [], tvos: null, applied: [] });
  });

  it("turns react-native-tvos' plugin on when the app has it, with the tvos block", () => {
    installTvosPlugin();

    expect(entries(undefined).tvos).toEqual([TVOS_PLUGIN, { isTV: true }]);
    expect(entries({ tvos: { isTV: false, androidTVRequired: true } }).tvos).toEqual([
      TVOS_PLUGIN,
      { isTV: false, androidTVRequired: true },
    ]);
  });

  it('leaves a platform, or Apple TV and Android TV, to an entry the app lists itself', () => {
    installTvosPlugin();

    const platforms = [['tizen', { package: 'tv.example.app' }]];
    expect(entries({ package: 'tv.example.app' }, ['webos', TVOS_PLUGIN])).toEqual({
      platforms,
      tvos: null,
      applied: platforms,
    });
  });

  it('refuses a block for what the app lists itself', () => {
    installTvosPlugin();

    expect(() => entries({ webos: {} }, ['webos'])).toThrow(
      `Invalid @kromatv/react-native-tv props: "webos" and the app's own webos entry both configure webos: keep one`,
    );
    expect(() => entries({ tvos: {} }, [TVOS_PLUGIN])).toThrow(
      `"tvos" and the app's own ${TVOS_PLUGIN} entry both configure Apple TV and Android TV: keep one`,
    );
  });

  it('refuses the tvos block in an app without the plugin', () => {
    expect(() => entries({ tvos: {} })).toThrow(`"tvos" needs ${TVOS_PLUGIN}: install it`);
  });

  it('refuses a block no platform of the app is named after, and names those it has', () => {
    expect(() => entries({ roku: {} })).toThrow(
      'Invalid @kromatv/react-native-tv props: "roku" is no TV platform this app depends on. It has tizen, tvos, webos',
    );
  });
});
