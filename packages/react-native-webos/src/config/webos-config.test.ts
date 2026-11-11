import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { resolveWebOSConfig } from './webos-config.ts';
import { EXPO_WEBOS_DEPRECATION, WEBOS_PLUGIN } from './webos-settings.ts';

const expo = (webos: unknown, extra: Record<string, unknown> = {}) => ({
  name: 'My App',
  version: '2.1.0',
  webos,
  ...extra,
});

describe('resolveWebOSConfig', () => {
  it('fills the label, version and privileges from the Expo config', () => {
    expect(resolveWebOSConfig(expo({ package: 'org.example.app' }))).toEqual({
      packageId: 'org.example.app',
      label: 'My App',
      version: '2.1.0',
      profile: undefined,
      privileges: ['http://webos.org/privilege/internet'],
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
      deprecation: EXPO_WEBOS_DEPRECATION,
    });
  });

  it('lets expo.webos override every Expo-wide field', () => {
    const config = resolveWebOSConfig(
      expo(
        { package: 'org.example.app', label: 'TV App', version: '3.0.1', icon: './tv.png' },
        { icon: './icon.png' },
      ),
    );

    expect(config).toMatchObject({ label: 'TV App', version: '3.0.1', icon: './tv.png' });
  });

  it('takes the Expo icon when expo.webos has none', () => {
    expect(resolveWebOSConfig(expo({ package: 'a.b' }, { icon: './icon.png' })).icon).toBe(
      './icon.png',
    );
  });

  it('expands a short privilege name and keeps a full one', () => {
    const config = resolveWebOSConfig(
      expo({
        package: 'a.b',
        privileges: ['internet', 'http://developer.samsung.com/privilege/network.public'],
      }),
    );

    expect(config.privileges).toEqual([
      'http://webos.org/privilege/internet',
      'http://developer.samsung.com/privilege/network.public',
    ]);
  });

  it('refuses an app without webOS settings, naming the plugin to add', () => {
    expect(() => resolveWebOSConfig({ name: 'My App' })).toThrow(`add ["${WEBOS_PLUGIN}"`);
  });

  it("reads the config plugin's props, with no deprecation", () => {
    const config = resolveWebOSConfig({
      name: 'My App',
      plugins: ['expo-video', [WEBOS_PLUGIN, { package: 'org.example.app', label: 'TV' }]],
    });

    expect(config).toMatchObject({ packageId: 'org.example.app', label: 'TV', deprecation: null });
  });

  it('names a bad prop after the plugin', () => {
    const plugins = [[WEBOS_PLUGIN, { package: 'my-app' }]];

    expect(() => resolveWebOSConfig({ name: 'My App', plugins })).toThrow(
      `plugins["${WEBOS_PLUGIN}"].package:`,
    );
  });

  it('refuses settings in both the plugin and expo.webos', () => {
    const plugins = [[WEBOS_PLUGIN, { package: 'a.b' }]];

    expect(() => resolveWebOSConfig(expo({ package: 'a.b' }, { plugins }))).toThrow(
      'remove expo.webos',
    );
  });

  it('reads expo.webos when the plugin is listed without props, as expo install adds it', () => {
    const config = resolveWebOSConfig(expo({ package: 'a.b' }, { plugins: [WEBOS_PLUGIN] }));

    expect(config.deprecation).toBe(EXPO_WEBOS_DEPRECATION);
  });

  it('finds the plugin under another name the app gives it', () => {
    const plugins = [['../app.plugin.js', { package: 'a.b' }]];

    expect(
      resolveWebOSConfig({ name: 'x', plugins }, (name) => name === '../app.plugin.js').packageId,
    ).toBe('a.b');
  });

  it('names the field of a package id webOS would refuse', () => {
    expect(() => resolveWebOSConfig(expo({ package: 'my-app' }))).toThrow(/^expo\.webos\.package:/);
  });

  it('refuses an unknown key, so a typo does not pass silently', () => {
    expect(() => resolveWebOSConfig(expo({ package: 'a.b', lable: 'x' }))).toThrow(FixableError);
  });

  it('refuses an Expo version webOS cannot read unless expo.webos.version replaces it', () => {
    expect(() => resolveWebOSConfig(expo({ package: 'a.b' }, { version: '1.0.0-beta' }))).toThrow(
      'set expo.webos.version',
    );
    expect(
      resolveWebOSConfig(expo({ package: 'a.b', version: '1.0.0' }, { version: '1.0.0-beta' }))
        .version,
    ).toBe('1.0.0');
  });

  it('reads the locales Intl formats in', () => {
    expect(resolveWebOSConfig(expo({ package: 'a.b', locales: ['fr', 'pt-BR'] })).locales).toEqual([
      'fr',
      'pt-BR',
    ]);
  });

  it('refuses a locale that is not a language tag', () => {
    expect(() => resolveWebOSConfig(expo({ package: 'a.b', locales: ['fr_FR'] }))).toThrow(
      /^expo\.webos\.locales\.0:/,
    );
  });

  it('reads the platform a package targets', () => {
    expect(resolveWebOSConfig(expo({ package: 'a.b', platform: '9.0' })).platform).toBe('9.0');
  });
});
