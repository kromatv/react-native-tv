import { homedir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { cacheDir } from './cache-dir.ts';

describe('cacheDir', () => {
  it('takes RN_WEBOS_CACHE_DIR as it is', () => {
    expect(cacheDir({ RN_WEBOS_CACHE_DIR: '/cache', XDG_CACHE_HOME: '/xdg' })).toBe('/cache');
  });

  it('lives under XDG_CACHE_HOME when it is set', () => {
    expect(cacheDir({ XDG_CACHE_HOME: '/xdg' })).toBe(join('/xdg', 'react-native-webos'));
  });

  it('defaults to ~/.cache/react-native-webos', () => {
    expect(cacheDir({})).toBe(join(homedir(), '.cache', 'react-native-webos'));
  });
});
