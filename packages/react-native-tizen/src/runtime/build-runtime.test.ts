import { mkdirSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import type { TizenTarget } from '../tizen/target.ts';
import { runtimeBuildDirs } from './build-runtime.ts';

const target: TizenTarget = { arch: 'armv7l', platformVersion: '9.0' };
const dirs: string[] = [];

function checkout(): { runtime: string; reactNative: string } {
  const root = mkdtempSync(join(tmpdir(), 'rn-tizen-checkout-'));
  dirs.push(root);
  const runtime = join(root, 'runtime');
  const reactNative = join(root, 'node_modules', 'react-native');
  mkdirSync(runtime, { recursive: true });
  mkdirSync(reactNative, { recursive: true });
  return { runtime, reactNative };
}

function reactNativeAt(dir: string) {
  return { name: 'react-native-tvos', version: '0.88.0-0', dir };
}

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('runtimeBuildDirs', () => {
  it('names the build after react-native, the target and a key of its sources', () => {
    const { runtime, reactNative } = checkout();

    const { root } = runtimeBuildDirs(reactNativeAt(reactNative), target, runtime);

    expect(root).toMatch(/[/\\]react-native-tvos-0\.88\.0-0-armv7l-tizen9\.0-[0-9a-f]{12}$/);
  });

  it('keeps one build for the same sources', () => {
    const { runtime, reactNative } = checkout();

    const first = runtimeBuildDirs(reactNativeAt(reactNative), target, runtime);
    const second = runtimeBuildDirs(reactNativeAt(reactNative), target, runtime);

    expect(second.cmake).toBe(first.cmake);
  });

  it('gives two checkouts of the same react-native version their own build', () => {
    const one = checkout();
    const other = checkout();

    const first = runtimeBuildDirs(reactNativeAt(one.reactNative), target, one.runtime);
    const second = runtimeBuildDirs(reactNativeAt(other.reactNative), target, other.runtime);

    expect(second.cmake).not.toBe(first.cmake);
  });
});
