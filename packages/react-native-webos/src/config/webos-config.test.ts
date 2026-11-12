import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { resolveWebOSConfig, RUNTIME_PERMISSIONS } from './webos-config.ts';
import { WEBOS_PLUGIN } from './webos-settings.ts';

const expo = (props: unknown, extra: Record<string, unknown> = {}) => ({
  name: 'My App',
  version: '2.1.0',
  plugins: [[WEBOS_PLUGIN, props]],
  ...extra,
});

const WHERE = `plugins["${WEBOS_PLUGIN}"]`;

describe('resolveWebOSConfig', () => {
  it('fills the title, version and permissions from the Expo config', () => {
    expect(resolveWebOSConfig(expo({ package: 'org.example.app' }))).toEqual({
      packageId: 'org.example.app',
      title: 'My App',
      version: '2.1.0',
      vendor: undefined,
      icon: undefined,
      iconColor: undefined,
      splash: undefined,
      permissions: [...RUNTIME_PERMISSIONS],
      locales: ['en'],
    });
  });

  it("lets the plugin's props override every Expo-wide field", () => {
    const config = resolveWebOSConfig(
      expo(
        {
          package: 'org.example.app',
          title: 'TV App',
          version: '3.0.1',
          icon: './tv.png',
          vendor: 'Example',
          iconColor: '#202020',
          splash: './splash.png',
        },
        { icon: './icon.png' },
      ),
    );

    expect(config).toMatchObject({
      title: 'TV App',
      version: '3.0.1',
      icon: './tv.png',
      vendor: 'Example',
      iconColor: '#202020',
      splash: './splash.png',
    });
  });

  it('takes the Expo icon when the plugin has none, and refuses one that is not a PNG', () => {
    expect(resolveWebOSConfig(expo({ package: 'a.b' }, { icon: './icon.png' })).icon).toBe(
      './icon.png',
    );
    expect(() => resolveWebOSConfig(expo({ package: 'a.b' }, { icon: './icon.jpg' }))).toThrow(
      `expo.icon: ./icon.jpg: expected the path of a PNG; set ${WHERE}.icon`,
    );
  });

  it("adds the app's ACG groups to the runtime's own, once each", () => {
    const config = resolveWebOSConfig(
      expo({ package: 'a.b', permissions: ['network.query', 'settings.query'] }),
    );

    expect(config.permissions).toEqual([...RUNTIME_PERMISSIONS, 'network.query']);
  });

  it('refuses an app without webOS settings, naming the plugin to add', () => {
    expect(() => resolveWebOSConfig({ name: 'My App' })).toThrow(`add ["${WEBOS_PLUGIN}"`);
    expect(() => resolveWebOSConfig({ name: 'My App', plugins: [WEBOS_PLUGIN] })).toThrow(
      `add ["${WEBOS_PLUGIN}"`,
    );
  });

  it('finds the plugin among the others, and under another name the app gives it', () => {
    const listed = resolveWebOSConfig({
      name: 'My App',
      plugins: ['expo-video', [WEBOS_PLUGIN, { package: 'org.example.app', title: 'TV' }]],
    });
    const plugins = [['../app.plugin.js', { package: 'a.b' }]];

    expect(listed).toMatchObject({ packageId: 'org.example.app', title: 'TV' });
    expect(
      resolveWebOSConfig({ name: 'x', plugins }, (name) => name === '../app.plugin.js').packageId,
    ).toBe('a.b');
  });

  it.each([
    ['My_App', 'lowercase letters'],
    ['.app', 'lowercase letters'],
    ['a', 'lowercase letters'],
    ['com.webos.app.mine', 'LG keeps those'],
    ['com.lge.mine', 'LG keeps those'],
    ['com.palmdts.mine', 'LG keeps those'],
  ])('refuses the app id %s, naming the prop', (id, reason) => {
    const refusal = () => resolveWebOSConfig(expo({ package: id }));

    expect(refusal).toThrow(`${WHERE}.package:`);
    expect(refusal).toThrow(reason);
  });

  it('takes the ids LG takes', () => {
    for (const id of ['com.example.my-app', 'a1', '3d.viewer', 'org.example.app2']) {
      expect(resolveWebOSConfig(expo({ package: id })).packageId).toBe(id);
    }
  });

  it('refuses an unknown key, so a typo does not pass silently', () => {
    expect(() => resolveWebOSConfig(expo({ package: 'a.b', titel: 'x' }))).toThrow(FixableError);
  });

  it("refuses an Expo version webOS cannot read unless the plugin's version replaces it", () => {
    expect(() => resolveWebOSConfig(expo({ package: 'a.b' }, { version: '1.0.0-beta' }))).toThrow(
      `set ${WHERE}.version`,
    );
    expect(() => resolveWebOSConfig(expo({ package: 'a.b', version: '1.02.0' }))).toThrow(
      `${WHERE}.version:`,
    );
    expect(
      resolveWebOSConfig(expo({ package: 'a.b', version: '1.0.0' }, { version: '1.0.0-beta' }))
        .version,
    ).toBe('1.0.0');
    expect(
      resolveWebOSConfig({ name: 'x', plugins: [[WEBOS_PLUGIN, { package: 'a.b' }]] }),
    ).toMatchObject({
      version: '1.0.0',
    });
  });

  it("refuses a title over LG's 20 characters, from the plugin or from expo.name", () => {
    const long = 'A Title Of Twenty-One';

    expect(() => resolveWebOSConfig(expo({ package: 'a.b', title: long }))).toThrow(
      `${WHERE}.title: expected 20 characters or fewer`,
    );
    expect(() => resolveWebOSConfig(expo({ package: 'a.b' }, { name: long }))).toThrow(
      `set ${WHERE}.title`,
    );
    expect(resolveWebOSConfig(expo({ package: 'a.b', title: 'Short' }, { name: long })).title).toBe(
      'Short',
    );
  });

  it('reads the locales Intl formats in', () => {
    expect(resolveWebOSConfig(expo({ package: 'a.b', locales: ['fr', 'pt-BR'] })).locales).toEqual([
      'fr',
      'pt-BR',
    ]);
  });

  it('refuses a locale that is not a language tag', () => {
    expect(() => resolveWebOSConfig(expo({ package: 'a.b', locales: ['fr_FR'] }))).toThrow(
      `${WHERE}.locales.0:`,
    );
  });
});
