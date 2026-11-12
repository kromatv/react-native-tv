import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { isTizenPluginEntry } from './plugin-entry.ts';

describe('isTizenPluginEntry', () => {
  const example = join(PACKAGE_ROOT, 'example');

  it("takes the package's name and a path to its app.plugin.js", () => {
    expect(isTizenPluginEntry(example, '@kromatv/react-native-tizen')).toBe(true);
    expect(isTizenPluginEntry(example, '../app.plugin.js')).toBe(true);
  });

  it('refuses other plugins and paths', () => {
    expect(isTizenPluginEntry(example, 'expo-video')).toBe(false);
    expect(isTizenPluginEntry(example, './app.plugin.js')).toBe(false);
    expect(isTizenPluginEntry(example, '../src/config/config-plugin.ts')).toBe(false);
    expect(isTizenPluginEntry(example, '/elsewhere/app.plugin.js')).toBe(false);
  });
});
