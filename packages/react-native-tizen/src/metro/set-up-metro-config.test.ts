import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { setUpMetroConfig } from './set-up-metro-config.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function app(files: Record<string, string> = {}): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-metro-setup-'));
  dirs.push(dir);
  for (const [name, text] of Object.entries(files)) {
    mkdirSync(join(dir, name, '..'), { recursive: true });
    writeFileSync(join(dir, name), text);
  }
  return dir;
}

const read = (dir: string, name: string) => readFileSync(join(dir, name), 'utf8');

const EXPO_TEMPLATE = [
  "const { getDefaultConfig } = require('expo/metro-config');",
  '',
  "/** @type {import('expo/metro-config').MetroConfig} */",
  'const config = getDefaultConfig(__dirname);',
  '',
  'module.exports = config;',
  '',
].join('\n');

describe('setUpMetroConfig', () => {
  it('writes metro.config.mjs when the app has no Metro config, once', () => {
    const dir = app();

    expect(setUpMetroConfig(dir)).toEqual({ kind: 'written', file: join(dir, 'metro.config.mjs') });
    expect(read(dir, 'metro.config.mjs')).toBe(
      [
        "import { getDefaultConfig } from 'expo/metro-config';",
        "import { withTizen } from '@kromatv/react-native-tizen/metro';",
        'export default withTizen(getDefaultConfig(import.meta.dirname));',
        '',
      ].join('\n'),
    );
    expect(setUpMetroConfig(dir)).toMatchObject({ kind: 'present' });
  });

  it("wraps Expo's CommonJS template, its import after the last require", () => {
    const dir = app({ 'metro.config.js': EXPO_TEMPLATE });

    expect(setUpMetroConfig(dir)).toMatchObject({ kind: 'wrapped' });
    expect(read(dir, 'metro.config.js').split('\n')).toEqual([
      "const { getDefaultConfig } = require('expo/metro-config');",
      "const { withTizen } = require('@kromatv/react-native-tizen/metro');",
      '',
      "/** @type {import('expo/metro-config').MetroConfig} */",
      'const config = getDefaultConfig(__dirname);',
      '',
      'module.exports = withTizen(config);',
      '',
    ]);
    expect(setUpMetroConfig(dir)).toMatchObject({ kind: 'present' });
  });

  it('wraps an ES module config, and one in .config/', () => {
    const esm = app({
      'metro.config.mjs':
        "import { getDefaultConfig } from 'expo/metro-config';\n\nexport default getDefaultConfig(import.meta.dirname)\n",
    });
    const hidden = app({
      '.config/metro.js': "'use strict';\nmodule.exports = { resolver: {} };\n",
    });

    setUpMetroConfig(esm);
    setUpMetroConfig(hidden);

    expect(read(esm, 'metro.config.mjs')).toBe(
      "import { getDefaultConfig } from 'expo/metro-config';\nimport { withTizen } from '@kromatv/react-native-tizen/metro';\n\nexport default withTizen(getDefaultConfig(import.meta.dirname));\n",
    );
    expect(read(hidden, '.config/metro.js')).toBe(
      "'use strict';\nconst { withTizen } = require('@kromatv/react-native-tizen/metro');\nmodule.exports = withTizen({ resolver: {} });\n",
    );
  });

  it.each([
    ['a multi-line export', 'module.exports = {\n  resolver: {},\n};\n', 'config'],
    ['a function config', 'module.exports = (base) => base;\n', '(base) => base'],
    ['code after the export', 'module.exports = config;\nconsole.log(1);\n', 'config'],
  ])('prints the two lines to add for %s', (_, text, value) => {
    const dir = app({ 'metro.config.js': text });

    expect(setUpMetroConfig(dir)).toEqual({
      kind: 'manual',
      file: join(dir, 'metro.config.js'),
      lines: [
        "const { withTizen } = require('@kromatv/react-native-tizen/metro');",
        `module.exports = withTizen(${value});`,
      ],
    });
    expect(read(dir, 'metro.config.js')).toBe(text);
  });

  it('prints ES module lines for a JSON config or a package.json metro key', () => {
    const json = app({ 'metro.config.json': '{}' });
    const pkg = app({ 'package.json': '{ "metro": { "resetCache": true } }' });

    expect(setUpMetroConfig(json)).toMatchObject({
      kind: 'manual',
      lines: [
        "import { withTizen } from '@kromatv/react-native-tizen/metro';",
        'export default withTizen(config);',
      ],
    });
    expect(setUpMetroConfig(pkg)).toMatchObject({
      kind: 'manual',
      file: join(pkg, 'package.json'),
    });
  });

  it('leaves the config of the workspace above the app to the user', () => {
    const workspace = app({
      'package.json': '{ "workspaces": ["apps/*"] }',
      'metro.config.mjs': 'export default config;\n',
      'apps/tv/package.json': '{ "name": "tv" }',
    });
    const outside = app({ 'apps/tv/package.json': '{ "name": "tv" }', 'metro.config.js': '' });

    expect(setUpMetroConfig(join(workspace, 'apps', 'tv'))).toEqual({
      kind: 'manual',
      file: join(workspace, 'metro.config.mjs'),
      lines: [
        "import { withTizen } from '@kromatv/react-native-tizen/metro';",
        'export default withTizen(config);',
      ],
    });
    expect(setUpMetroConfig(join(outside, 'apps', 'tv'))).toMatchObject({ kind: 'written' });
  });
});
