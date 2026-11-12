import { realpathSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { EXPO_CLI_ENTRY_FILE, installExpoCli } from './expo-cli.fixtures.ts';
import { expoCliPackage } from './expo-cli-package.ts';

let app: TestApp;

beforeEach(() => {
  app = testApp();
});

afterEach(() => {
  app.remove();
});

describe('expoCliPackage', () => {
  it("finds the Expo CLI the app's expo resolves, its version and its entry", () => {
    installExpoCli(app);
    const root = realpathSync(app.root);

    expect(expoCliPackage(app.root)).toEqual({
      dir: join(root, 'node_modules/@expo/cli'),
      version: '58.1.20',
      entry: join(root, EXPO_CLI_ENTRY_FILE),
    });
  });

  it('finds none in an app without expo, or whose Expo CLI has no entry', () => {
    expect(expoCliPackage(app.root)).toBeNull();
    installExpoCli(app, null);
    expect(expoCliPackage(app.root)).toBeNull();
  });

  it('finds none when the Expo CLI names no version', () => {
    installExpoCli(app);
    app.write('node_modules/@expo/cli/package.json', {
      name: '@expo/cli',
      main: 'build/src/index.js',
    });

    expect(expoCliPackage(app.root)).toBeNull();
  });
});
