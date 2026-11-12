import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { addPostinstallScript } from './postinstall-script.ts';

let app: TestApp;
let manifest: string;

beforeEach(() => {
  app = testApp();
  manifest = join(app.root, 'package.json');
});

afterEach(() => {
  app.remove();
});

function scripts(): unknown {
  return JSON.parse(readFileSync(manifest, 'utf8')).scripts;
}

describe('addPostinstallScript', () => {
  it('adds the script to a manifest with none, and keeps the rest of the file', () => {
    app.write('package.json', '{\n  "name": "app",\n  "dependencies": { "expo": "58.0.23" }\n}\n');

    expect(addPostinstallScript(manifest)).toBe(true);
    expect(readFileSync(manifest, 'utf8')).toBe(
      '{\n  "name": "app",\n  "dependencies": { "expo": "58.0.23" },\n  "scripts": {\n    "postinstall": "rn-tv expo-patch"\n  }\n}\n',
    );
  });

  it('runs after what postinstall runs already', () => {
    app.write('package.json', { scripts: { start: 'expo start', postinstall: 'patch-package ' } });

    expect(addPostinstallScript(manifest)).toBe(true);
    expect(scripts()).toEqual({
      start: 'expo start',
      postinstall: 'patch-package && rn-tv expo-patch',
    });
  });

  it('takes the place of a postinstall that runs nothing', () => {
    app.write('package.json', { scripts: { postinstall: ' ' } });

    expect(addPostinstallScript(manifest)).toBe(true);
    expect(scripts()).toEqual({ postinstall: 'rn-tv expo-patch' });
  });

  it.each([
    [{ postinstall: 'rn-tv expo-patch' }],
    [{ postinstall: 'patch-package && rn-tv expo-patch && echo done' }],
    [{ postinstall: 'npm run patches', patches: 'rn-tv expo-patch' }],
  ])('changes nothing when %j runs the command already', (before) => {
    app.write('package.json', { scripts: before });
    const text = readFileSync(manifest, 'utf8');

    expect(addPostinstallScript(manifest)).toBe(false);
    expect(readFileSync(manifest, 'utf8')).toBe(text);
  });

  it('says so for a manifest it cannot read, and for scripts it cannot add to', () => {
    expect(() => addPostinstallScript(manifest)).toThrow(
      `${manifest} is not a package.json rn-tv can read.`,
    );
    app.write('package.json', { scripts: 'none' });
    expect(() => addPostinstallScript(manifest)).toThrow(
      `${manifest} has scripts rn-tv cannot add to.`,
    );
  });
});
