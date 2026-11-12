import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { isWebOSPluginEntry } from './plugin-entry.ts';

describe('isWebOSPluginEntry', () => {
  const example = join(PACKAGE_ROOT, 'example');

  it("takes the package's name and a path to its app.plugin.js", () => {
    expect(isWebOSPluginEntry(example, '@kromatv/react-native-webos')).toBe(true);
    expect(isWebOSPluginEntry(example, '../app.plugin.js')).toBe(true);
  });

  it('refuses other plugins and paths', () => {
    expect(isWebOSPluginEntry(example, 'expo-video')).toBe(false);
    expect(isWebOSPluginEntry(example, './app.plugin.js')).toBe(false);
    expect(isWebOSPluginEntry(example, '../src/config/config-plugin.ts')).toBe(false);
    expect(isWebOSPluginEntry(example, '/elsewhere/app.plugin.js')).toBe(false);
  });
});
