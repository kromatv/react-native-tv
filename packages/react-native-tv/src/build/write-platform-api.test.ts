import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { writePlatformApi } from './write-platform-api.ts';

let app: TestApp;

beforeEach(() => {
  app = testApp();
});

afterEach(() => {
  app.remove();
});

function platform(name: string, platformName: string, modules: readonly string[]): void {
  app.write(`node_modules/${name}/package.json`, {
    name,
    exports: {
      './metro': './metro.js',
      './package.json': './package.json',
      ...Object.fromEntries(modules.map((module) => [`./${module}`, `./${module}.js`])),
    },
    reactNativeTvPlatform: { metro: './metro', platform: platformName },
  });
}

describe('writePlatformApi', () => {
  it('writes a module and its types for each API module of each platform the package depends on', () => {
    app.dependOn('tv-webos', 'tv-tizen', 'plain', 'missing');
    platform('tv-tizen', 'tizen', ['preview', 'tizen']);
    platform('tv-webos', 'webos', ['webos']);
    app.write('node_modules/plain/package.json', { name: 'plain', exports: { './api': './a.js' } });
    const out = join(app.root, 'out');

    expect(writePlatformApi(join(app.root, 'package.json'), out)).toEqual([
      'tizen/preview',
      'tizen',
      'webos',
    ]);
    expect(readdirSync(out).sort()).toEqual(['tizen', 'tizen.d.ts', 'tizen.js', 'webos.d.ts', 'webos.js']);
    expect(readFileSync(join(out, 'tizen/preview.js'), 'utf8')).toBe(
      'export * from "tv-tizen/preview";\n',
    );
    expect(readFileSync(join(out, 'tizen/preview.d.ts'), 'utf8')).toBe(
      'export * from "tv-tizen/preview";\n',
    );
  });

  it('writes nothing for a package with no dependencies, or no package.json', () => {
    const out = join(app.root, 'out');

    expect(writePlatformApi(join(app.root, 'package.json'), out)).toEqual([]);
    app.write('package.json', {});
    expect(writePlatformApi(join(app.root, 'package.json'), out)).toEqual([]);
  });
});
