import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import type { TizenAppConfig } from '../config/tizen-config.ts';
import { SHELL_DIR } from '../package-paths.ts';
import { platformProfile } from '../tizen/platform-profiles.ts';
import { manifestValues, projectValues, renderTemplate } from './templates.ts';
import { escapeXml } from './xml.ts';

const config: TizenAppConfig = {
  packageId: 'org.rntizen.example',
  label: 'Tom & Jerry <TV>',
  version: '1.2.3',
  profile: undefined,
  privileges: [
    'http://tizen.org/privilege/internet',
    'http://developer.samsung.com/privilege/network.public',
  ],
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
  deprecation: null,
};

const shellFile = (name: string) => readFileSync(join(SHELL_DIR, name), 'utf8');

describe('renderTemplate', () => {
  it('fills every placeholder', () => {
    expect(renderTemplate('{{a}} and {{b}}', { a: '1', b: '2' })).toBe('1 and 2');
  });

  it('throws on a placeholder it has no value for', () => {
    expect(() => renderTemplate('{{missing}}', {})).toThrow('{{missing}}');
  });
});

describe('escapeXml', () => {
  it('escapes the five XML special characters', () => {
    expect(escapeXml(`<a href="x">'&'</a>`)).toBe(
      '&lt;a href=&quot;x&quot;&gt;&apos;&amp;&apos;&lt;/a&gt;',
    );
  });
});

describe('the shell manifest', () => {
  const manifest = renderTemplate(
    shellFile('tizen-manifest.xml'),
    manifestValues(config, platformProfile('9.0')),
  );

  it('names the package, its version and its platform as x.y', () => {
    expect(manifest).toContain(
      '<manifest package="org.rntizen.example" version="1.2.3" api-version="9.0"',
    );
    expect(manifest).toContain('appid="org.rntizen.example"');
  });

  it('declares the TV profile', () => {
    expect(manifest).toContain('<profile name="tv" />');
  });

  it('escapes the label', () => {
    expect(manifest).toContain('<label>Tom &amp; Jerry &lt;TV&gt;</label>');
  });

  it('lists one element per privilege', () => {
    expect(manifest).toContain(
      '    <privilege>http://tizen.org/privilege/internet</privilege>\n    <privilege>http://developer.samsung.com/privilege/network.public</privilege>',
    );
  });
});

describe('the shell project', () => {
  it('targets TizenFX API 8 and the Tizen.NET version whatever the floor', () => {
    const project = renderTemplate(
      shellFile('ReactNativeTizen.csproj'),
      projectValues(platformProfile('9.0'), config),
    );

    expect(project).toContain('<TargetFramework>tizen80</TargetFramework>');
    expect(project).toContain('<TizenNetPackageVersion>12.0.0.18510</TizenNetPackageVersion>');
  });
});
