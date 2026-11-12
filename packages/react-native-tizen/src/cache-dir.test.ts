import { homedir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { cacheDir } from './cache-dir.ts';

describe('cacheDir', () => {
  it('takes RN_TIZEN_CACHE_DIR as it is', () => {
    expect(cacheDir({ RN_TIZEN_CACHE_DIR: '/cache', XDG_CACHE_HOME: '/xdg' })).toBe('/cache');
  });

  it('lives under XDG_CACHE_HOME when it is set', () => {
    expect(cacheDir({ XDG_CACHE_HOME: '/xdg' })).toBe(join('/xdg', 'react-native-tizen'));
  });

  it('defaults to ~/.cache/react-native-tizen', () => {
    expect(cacheDir({})).toBe(join(homedir(), '.cache', 'react-native-tizen'));
  });
});
