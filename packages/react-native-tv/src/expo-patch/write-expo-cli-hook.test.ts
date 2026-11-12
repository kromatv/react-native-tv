import {
  chmodSync,
  existsSync,
  linkSync,
  mkdirSync,
  readFileSync,
  readdirSync,
  statSync,
} from 'node:fs';
import { dirname, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { EXPO_CLI_ENTRY, EXPO_CLI_ENTRY_FILE, installExpoCli } from './expo-cli.fixtures.ts';
import { addExpoCliHook } from './expo-cli-hook.ts';
import { writeExpoCliHook } from './write-expo-cli-hook.ts';

let app: TestApp;
let entry: string;

beforeEach(() => {
  app = testApp();
  installExpoCli(app);
  entry = join(app.root, EXPO_CLI_ENTRY_FILE);
});

afterEach(() => {
  app.remove();
});

describe('writeExpoCliHook', () => {
  it('adds the hook, keeps the mode of the file and leaves nothing beside it', () => {
    chmodSync(entry, 0o755);

    expect(writeExpoCliHook(entry)).toBe(true);
    expect(readFileSync(entry, 'utf8')).toBe(addExpoCliHook(EXPO_CLI_ENTRY));
    expect(statSync(entry).mode & 0o777).toBe(0o755);
    expect(readdirSync(dirname(entry))).toEqual(['index.js']);
  });

  it('writes a new file, so a hard link to the old one keeps what Expo shipped', () => {
    const stored = join(app.root, 'store-copy.js');
    linkSync(entry, stored);

    expect(writeExpoCliHook(entry)).toBe(true);
    expect(readFileSync(stored, 'utf8')).toBe(EXPO_CLI_ENTRY);
  });

  it('adds the hook once', () => {
    writeExpoCliHook(entry);

    expect(writeExpoCliHook(entry)).toBe(true);
    expect(readFileSync(entry, 'utf8')).toBe(addExpoCliHook(EXPO_CLI_ENTRY));
  });

  it('leaves an entry it does not know as it was', () => {
    app.write(EXPO_CLI_ENTRY_FILE, 'module.exports = {};');

    expect(writeExpoCliHook(entry)).toBe(false);
    expect(readFileSync(entry, 'utf8')).toBe('module.exports = {};');
  });

  it('leaves the entry and nothing beside it when the new file cannot be written', () => {
    mkdirSync(`${entry}.rn-tv-expo-patch`);

    expect(() => writeExpoCliHook(entry)).toThrow(/EISDIR/);
    expect(readFileSync(entry, 'utf8')).toBe(EXPO_CLI_ENTRY);
    expect(existsSync(`${entry}.rn-tv-expo-patch`)).toBe(false);
  });
});
