import { describe, expect, it } from 'vitest';
import { platformApiModules } from './platform-api.ts';

describe('platformApiModules', () => {
  it("lists each plain subpath of a platform's exports under the platform's name, but its Metro entry", () => {
    const manifest = {
      name: '@scope/tv-tizen',
      exports: {
        '.': './index.js',
        './metro': './metro.js',
        './voice': './voice.js',
        './preview-json': './preview-json.js',
        './tizen': './tizen.js',
        './app.plugin.js': './app.plugin.js',
        './package.json': './package.json',
        './deep/path': './deep.js',
      },
      reactNativeTvPlatform: { metro: './metro', platform: 'tizen' },
    };

    expect(platformApiModules(manifest)).toEqual([
      { file: 'tizen/preview-json', specifier: '@scope/tv-tizen/preview-json' },
      { file: 'tizen', specifier: '@scope/tv-tizen/tizen' },
      { file: 'tizen/voice', specifier: '@scope/tv-tizen/voice' },
    ]);
  });

  it('lists none for a package that names no platform, has no exports or is not a manifest', () => {
    const exports = { './voice': './voice.js' };

    expect(platformApiModules({ name: 'plain', exports })).toEqual([]);
    expect(
      platformApiModules({ name: 'metro-only', exports, reactNativeTvPlatform: { metro: './metro' } }),
    ).toEqual([]);
    expect(
      platformApiModules({ name: 'tv', reactNativeTvPlatform: { metro: './metro', platform: 'tv' } }),
    ).toEqual([]);
    expect(platformApiModules(null)).toEqual([]);
  });
});
