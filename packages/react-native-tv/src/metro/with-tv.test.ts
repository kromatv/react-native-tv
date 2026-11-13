import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { addsPlatform, testApp, type TestApp } from '../app.fixtures.ts';
import * as metro from './index.ts';
import { withTV } from './with-tv.ts';

let app: TestApp;

beforeEach(() => {
  app = testApp();
});

afterEach(() => {
  vi.restoreAllMocks();
  app.remove();
});

describe('withTV', () => {
  it('applies the wrapper of every TV platform the app depends on, in the order of their names', () => {
    app.dependOn('tv-webos', 'tv-tizen', 'plain');
    app.platformPackage('tv-webos', {}, addsPlatform('webos'));
    app.platformPackage('tv-tizen', {}, addsPlatform('tizen'));
    app.write('node_modules/plain/package.json', { name: 'plain' });

    expect(withTV({ projectRoot: app.root, platforms: ['ios'], watchFolders: ['x'] })).toEqual({
      projectRoot: app.root,
      platforms: ['ios', 'tizen', 'webos'],
      watchFolders: ['x'],
    });
  });

  it('applies the wrapper of a platform this package brings, loaded from this package', () => {
    app.dependOn('@kromatv/react-native-tv', 'tv-webos');
    app.platformPackage('tv-webos', {}, addsPlatform('webos'));
    app.bring({ 'tv-tizen': {} }, () => addsPlatform('tizen'));

    expect(withTV({ projectRoot: app.root, platforms: ['ios'] })).toEqual({
      projectRoot: app.root,
      platforms: ['ios', 'tizen', 'webos'],
    });
  });

  it('leaves a platform the config knows already, so a second call changes nothing', () => {
    const adds = (platform: string) =>
      `exports.withTvPlatform = (config) => ({ ...config, resolver: { platforms: [...(config.resolver?.platforms ?? []), '${platform}'] } });`;
    app.dependOn('tv-webos', 'tv-tizen');
    app.platformPackage('tv-webos', { platform: 'webos' }, adds('webos'));
    app.platformPackage('tv-tizen', { platform: 'tizen' }, adds('tizen'));
    const once = withTV({ projectRoot: app.root, resolver: { platforms: ['ios', 'webos'] } });

    expect(once.resolver).toEqual({ platforms: ['ios', 'webos', 'tizen'] });
    expect(withTV(once)).toBe(once);
  });

  it('reads the app in the current directory when the config names no project root', () => {
    app.dependOn('tv-webos');
    app.platformPackage('tv-webos', {}, addsPlatform('webos'));
    vi.spyOn(process, 'cwd').mockReturnValue(app.root);

    expect(withTV({})).toEqual({ platforms: ['webos'] });
  });

  it('returns the config of an app with no TV platform as it is', () => {
    app.dependOn('plain');
    const config = { projectRoot: app.root };

    expect(withTV(config)).toBe(config);
  });

  it('says which package exports no wrapper', () => {
    app.dependOn('tv-empty');
    app.platformPackage('tv-empty', {}, 'exports.withTvPlatform = 1;');

    expect(() => withTV({ projectRoot: app.root })).toThrow(
      'tv-empty declares a TV platform, but tv-empty/metro exports no withTvPlatform function',
    );
  });

  it('is what the Metro entry exports', () => {
    expect(Object.keys(metro)).toEqual(['withTV']);
  });
});
