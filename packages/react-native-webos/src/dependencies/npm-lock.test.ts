import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { unlockUpstreamReactNative } from './npm-lock.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function project(lock?: unknown): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-lock-'));
  dirs.push(dir);
  if (lock !== undefined)
    writeFileSync(join(dir, 'package-lock.json'), JSON.stringify(lock, null, 2));
  return dir;
}

const lockOf = (dir: string) => JSON.parse(readFileSync(join(dir, 'package-lock.json'), 'utf8'));

describe('unlockUpstreamReactNative', () => {
  it('drops the locked upstream react-native and keeps the alias and everything else', () => {
    const dir = project({
      lockfileVersion: 3,
      packages: {
        '': { name: 'tv-app' },
        'node_modules/react-native': { name: 'react-native-tvos', version: '0.88.0-0' },
        'node_modules/react-native/node_modules/react-native': { version: '0.88.0', peer: true },
        'node_modules/react-native-svg': { version: '15.16.0' },
      },
    });

    expect(unlockUpstreamReactNative(dir)).toBe(true);
    expect(lockOf(dir)).toEqual({
      lockfileVersion: 3,
      packages: {
        '': { name: 'tv-app' },
        'node_modules/react-native': { name: 'react-native-tvos', version: '0.88.0-0' },
        'node_modules/react-native-svg': { version: '15.16.0' },
      },
    });
    expect(unlockUpstreamReactNative(dir)).toBe(false);
  });

  it('leaves a project alone without a lockfile or with one it does not read', () => {
    const odd = project({ packages: ['not', 'a', 'map'] });

    expect(unlockUpstreamReactNative(project())).toBe(false);
    expect(unlockUpstreamReactNative(odd)).toBe(false);
    expect(unlockUpstreamReactNative(project({ lockfileVersion: 1 }))).toBe(false);
  });
});
