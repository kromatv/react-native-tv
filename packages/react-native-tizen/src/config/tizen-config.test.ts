import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { resolveTizenConfig } from './tizen-config.ts';
import { EXPO_TIZEN_DEPRECATION, TIZEN_PLUGIN } from './tizen-settings.ts';

const expo = (tizen: unknown, extra: Record<string, unknown> = {}) => ({
  name: 'My App',
  version: '2.1.0',
  tizen,
  ...extra,
});

describe('resolveTizenConfig', () => {
  it('fills the label, version and privileges from the Expo config', () => {
    expect(resolveTizenConfig(expo({ package: 'org.example.app' }))).toEqual({
      packageId: 'org.example.app',
      label: 'My App',
      version: '2.1.0',
      profile: undefined,
      privileges: ['http://tizen.org/privilege/internet'],
      platform: undefined,
      icon: undefined,
      locales: ['en'],
      features: {
        voice: null,
        preview: null,
        resolution: '1080p',
        backgroundMedia: false,
        pictureInPicture: false,
      },
      deprecation: EXPO_TIZEN_DEPRECATION,
    });
  });

  it('lets expo.tizen override every Expo-wide field', () => {
    const config = resolveTizenConfig(
      expo(
        { package: 'org.example.app', label: 'TV App', version: '3.0.1', icon: './tv.png' },
        { icon: './icon.png' },
      ),
    );

    expect(config).toMatchObject({ label: 'TV App', version: '3.0.1', icon: './tv.png' });
  });

  it('takes the Expo icon when expo.tizen has none', () => {
    expect(resolveTizenConfig(expo({ package: 'a.b' }, { icon: './icon.png' })).icon).toBe(
      './icon.png',
    );
  });

  it('expands a short privilege name and keeps a full one', () => {
    const config = resolveTizenConfig(
      expo({
        package: 'a.b',
        privileges: ['internet', 'http://developer.samsung.com/privilege/network.public'],
      }),
    );

    expect(config.privileges).toEqual([
      'http://tizen.org/privilege/internet',
      'http://developer.samsung.com/privilege/network.public',
    ]);
  });

  it('refuses an app without Tizen settings, naming the plugin to add', () => {
    expect(() => resolveTizenConfig({ name: 'My App' })).toThrow(`add ["${TIZEN_PLUGIN}"`);
  });

  it("reads the config plugin's props, with no deprecation", () => {
    const config = resolveTizenConfig({
      name: 'My App',
      plugins: ['expo-video', [TIZEN_PLUGIN, { package: 'org.example.app', label: 'TV' }]],
    });

    expect(config).toMatchObject({ packageId: 'org.example.app', label: 'TV', deprecation: null });
  });

  it('names a bad prop after the plugin', () => {
    const plugins = [[TIZEN_PLUGIN, { package: 'my-app' }]];

    expect(() => resolveTizenConfig({ name: 'My App', plugins })).toThrow(
      `plugins["${TIZEN_PLUGIN}"].package:`,
    );
  });

  it('refuses settings in both the plugin and expo.tizen', () => {
    const plugins = [[TIZEN_PLUGIN, { package: 'a.b' }]];

    expect(() => resolveTizenConfig(expo({ package: 'a.b' }, { plugins }))).toThrow(
      'remove expo.tizen',
    );
  });

  it('reads expo.tizen when the plugin is listed without props, as expo install adds it', () => {
    const config = resolveTizenConfig(expo({ package: 'a.b' }, { plugins: [TIZEN_PLUGIN] }));

    expect(config.deprecation).toBe(EXPO_TIZEN_DEPRECATION);
  });

  it('finds the plugin under another name the app gives it', () => {
    const plugins = [['../app.plugin.js', { package: 'a.b' }]];

    expect(
      resolveTizenConfig({ name: 'x', plugins }, (name) => name === '../app.plugin.js').packageId,
    ).toBe('a.b');
  });

  it('names the field of a package id Tizen would refuse', () => {
    expect(() => resolveTizenConfig(expo({ package: 'my-app' }))).toThrow(/^expo\.tizen\.package:/);
  });

  it('refuses an unknown key, so a typo does not pass silently', () => {
    expect(() => resolveTizenConfig(expo({ package: 'a.b', lable: 'x' }))).toThrow(FixableError);
  });

  it('refuses an Expo version Tizen cannot read unless expo.tizen.version replaces it', () => {
    expect(() => resolveTizenConfig(expo({ package: 'a.b' }, { version: '1.0.0-beta' }))).toThrow(
      'set expo.tizen.version',
    );
    expect(
      resolveTizenConfig(expo({ package: 'a.b', version: '1.0.0' }, { version: '1.0.0-beta' }))
        .version,
    ).toBe('1.0.0');
  });

  it('reads the locales Intl formats in', () => {
    expect(resolveTizenConfig(expo({ package: 'a.b', locales: ['fr', 'pt-BR'] })).locales).toEqual([
      'fr',
      'pt-BR',
    ]);
  });

  it('refuses a locale that is not a language tag', () => {
    expect(() => resolveTizenConfig(expo({ package: 'a.b', locales: ['fr_FR'] }))).toThrow(
      /^expo\.tizen\.locales\.0:/,
    );
  });

  it('reads the platform a package targets', () => {
    expect(resolveTizenConfig(expo({ package: 'a.b', platform: '9.0' })).platform).toBe('9.0');
  });
});
