import { realpathSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, TV_PACKAGE_DIR, type TestApp } from '../app.fixtures.ts';
import { tvPlatformPackages } from './tv-platform-packages.ts';

let app: TestApp;

beforeEach(() => {
  app = testApp();
});

afterEach(() => {
  app.remove();
});

describe('tvPlatformPackages', () => {
  it('finds the dependencies that declare a TV platform, by name, whatever kind of dependency', () => {
    app.write('package.json', {
      dependencies: { 'tv-b': '1.0.0', plain: '1.0.0', missing: '1.0.0' },
      devDependencies: { '@scope/tv-a': '1.0.0', 'tv-b': '1.0.0' },
    });
    app.platformPackage('tv-b', {});
    app.platformPackage('@scope/tv-a', {});
    app.platformPackage('not-a-dependency', {});
    app.write('node_modules/plain/package.json', { name: 'plain' });

    const from = join(app.root, 'package.json');
    expect(tvPlatformPackages(app.root)).toEqual([
      { name: '@scope/tv-a', metro: '@scope/tv-a/metro', from, platform: null, cli: null },
      { name: 'tv-b', metro: 'tv-b/metro', from, platform: null, cli: null },
    ]);
  });

  it('finds the platforms this package brings, where the app cannot reach them, from this package', () => {
    app.dependOn('@kromatv/react-native-tv');
    app.bring({ tizen: { platform: 'tizen', cli: 'rn-tizen' } });
    const tvPackage = join(realpathSync(app.root), TV_PACKAGE_DIR);

    expect(tvPlatformPackages(app.root)).toEqual([
      {
        name: 'tizen',
        metro: 'tizen/metro',
        from: join(tvPackage, 'package.json'),
        platform: 'tizen',
        cli: join(tvPackage, 'node_modules/tizen/cli.js'),
      },
    ]);
  });

  it("takes the app's copy of a platform over the one this package brings", () => {
    app.dependOn('@kromatv/react-native-tv');
    app.bring({ tizen: { platform: 'tizen' }, webos: { platform: 'webos' } });
    app.platformPackage('webos', { platform: 'webos' });

    expect(tvPlatformPackages(app.root).map(({ name, from }) => [name, from])).toEqual([
      ['tizen', join(realpathSync(app.root), TV_PACKAGE_DIR, 'package.json')],
      ['webos', join(app.root, 'package.json')],
    ]);
  });

  it('gives the platform a package names and the script of its command', () => {
    app.dependOn('webos');
    app.platformPackage('webos', { platform: 'webos', cli: 'rn-webos' });

    expect(tvPlatformPackages(app.root)).toEqual([
      {
        name: 'webos',
        metro: 'webos/metro',
        from: join(app.root, 'package.json'),
        platform: 'webos',
        cli: join(realpathSync(app.root), 'node_modules/webos/cli.js'),
      },
    ]);
  });

  it('gives no script for a command the package has no bin for, or one outside the package', () => {
    app.dependOn('unknown', 'outside', 'stringbin');
    app.platformPackage('unknown', { cli: 'rn-other' });
    app.write('node_modules/outside/package.json', {
      name: 'outside',
      bin: { out: '../../elsewhere.js' },
      reactNativeTvPlatform: { metro: './metro', cli: 'out' },
    });
    app.write('node_modules/stringbin/package.json', {
      name: 'stringbin',
      bin: 'cli.js',
      reactNativeTvPlatform: { metro: './metro', cli: 'stringbin' },
    });

    expect(tvPlatformPackages(app.root).map(({ name, cli }) => [name, cli])).toEqual([
      ['outside', null],
      ['stringbin', null],
      ['unknown', null],
    ]);
  });

  it('skips a declaration with no Metro subpath or a platform that is not a name, and a package that hides its package.json', () => {
    app.dependOn('absolute', 'hidden', 'broken', 'shouting');
    app.write('node_modules/absolute/package.json', {
      name: 'absolute',
      reactNativeTvPlatform: { metro: '/etc/passwd' },
    });
    app.write('node_modules/hidden/package.json', {
      name: 'hidden',
      exports: { '.': './index.js' },
      reactNativeTvPlatform: { metro: './metro' },
    });
    app.write('node_modules/broken/package.json', '{');
    app.platformPackage('shouting', { platform: 'Web OS' });

    expect(tvPlatformPackages(app.root)).toEqual([]);
  });

  it('finds none for an app with no package.json, or one that is not an object', () => {
    expect(tvPlatformPackages(app.root)).toEqual([]);
    app.write('package.json', '[]');
    expect(tvPlatformPackages(app.root)).toEqual([]);
  });
});
