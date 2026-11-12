import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { homedir, tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { lastTargetFile, lastTvTarget, rememberTvTarget } from './last-tv-target.ts';

let dir: string;
let file: string;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tv-last-'));
  file = join(dir, 'nested', 'last-target.json');
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('lastTargetFile', () => {
  it('is in the cache directory, which two variables move', () => {
    expect(lastTargetFile({})).toBe(join(homedir(), '.cache/react-native-tv/last-target.json'));
    expect(lastTargetFile({ XDG_CACHE_HOME: '/xdg' })).toBe(
      '/xdg/react-native-tv/last-target.json',
    );
    expect(lastTargetFile({ XDG_CACHE_HOME: '/xdg', RN_TV_CACHE_DIR: '/mine' })).toBe(
      '/mine/last-target.json',
    );
  });
});

describe('rememberTvTarget', () => {
  it('remembers one target per project, the latest', () => {
    expect(lastTvTarget(file, '/app')).toBeNull();
    rememberTvTarget(file, '/app', { platform: 'tizen', target: '10.0.0.7' });
    rememberTvTarget(file, '/other', { platform: 'webos', target: 'simulator' });
    rememberTvTarget(file, '/app', { platform: 'webos', target: '10.0.0.9' });

    expect(lastTvTarget(file, '/app')).toEqual({ platform: 'webos', target: '10.0.0.9' });
    expect(lastTvTarget(file, '/other')).toEqual({ platform: 'webos', target: 'simulator' });
  });

  it('keeps the hundred projects written last', () => {
    for (let index = 0; index < 102; index += 1) {
      rememberTvTarget(file, `/app-${index}`, { platform: 'tizen', target: 'tv' });
    }

    expect(lastTvTarget(file, '/app-0')).toBeNull();
    expect(lastTvTarget(file, '/app-1')).toBeNull();
    expect(lastTvTarget(file, '/app-2')).not.toBeNull();
    expect(lastTvTarget(file, '/app-101')).not.toBeNull();
  });

  it('starts over from a file it cannot read, and ignores one it cannot write', () => {
    writeFileSync(join(dir, 'broken.json'), '{');
    writeFileSync(join(dir, 'odd.json'), '{"/app":{"platform":1}}');
    writeFileSync(join(dir, 'huge.json'), `{"/app":"${'x'.repeat(1024 * 1024)}"}`);

    expect(lastTvTarget(join(dir, 'broken.json'), '/app')).toBeNull();
    expect(lastTvTarget(join(dir, 'odd.json'), '/app')).toBeNull();
    expect(lastTvTarget(join(dir, 'huge.json'), '/app')).toBeNull();
    expect(() =>
      rememberTvTarget(join(dir, 'broken.json', 'under-a-file.json'), '/app', {
        platform: 'tizen',
        target: 'tv',
      }),
    ).not.toThrow();
  });
});
