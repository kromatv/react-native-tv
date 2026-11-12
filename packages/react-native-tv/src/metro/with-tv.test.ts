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
