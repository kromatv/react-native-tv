import { describe, expect, it } from 'vitest';
import { packageInfo } from './package-info.ts';

const files = (name: string, text: string) => new Map([[name, Buffer.from(text)]]);

describe('packageInfo', () => {
  it('installs a .tpk under its package id and launches its UI app', () => {
    const manifest =
      '<manifest package="org.example.app" version="1.0.0"><ui-application appid="org.example.app.main" exec="App.dll"/></manifest>';

    expect(packageInfo(files('tizen-manifest.xml', manifest))).toEqual({
      kind: 'tpk',
      packageId: 'org.example.app',
      appId: 'org.example.app.main',
      installId: 'org.example.app',
    });
  });

  it('installs a .wgt under its application id, as Tizen Studio does', () => {
    const config =
      '<widget xmlns:tizen="http://tizen.org/ns/widgets"><tizen:application id="AbCdE12345.App" package="AbCdE12345" required_version="2.3"/></widget>';

    expect(packageInfo(files('config.xml', config))).toEqual({
      kind: 'wgt',
      packageId: 'AbCdE12345',
      appId: 'AbCdE12345.App',
      installId: 'AbCdE12345.App',
    });
  });

  it('refuses an id the TV command line could misread, and a package with no manifest', () => {
    const config = '<widget><tizen:application id="A;reboot" package="A"/></widget>';

    expect(() => packageInfo(files('config.xml', config))).toThrow();
    expect(() => packageInfo(files('index.html', ''))).toThrow(
      'neither a tizen-manifest.xml nor a config.xml',
    );
  });
});
