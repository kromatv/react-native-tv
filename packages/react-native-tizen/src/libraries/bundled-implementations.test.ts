import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { BUNDLED_IMPLEMENTATIONS_DIR, bundledImplementations } from './bundled-implementations.ts';

let dir: string | undefined;

afterEach(() => {
  if (dir !== undefined) rmSync(dir, { recursive: true, force: true });
  dir = undefined;
});

describe('bundledImplementations', () => {
  it('maps react-native-svg to the implementation this package ships', () => {
    expect(bundledImplementations().get('react-native-svg')).toBe(
      join(BUNDLED_IMPLEMENTATIONS_DIR, 'svg', 'CMakeLists.txt'),
    );
  });

  it('ignores a directory without a library.json', () => {
    dir = mkdtempSync(join(tmpdir(), 'rn-tizen-bundled-'));
    mkdirSync(join(dir, 'views'));
    mkdirSync(join(dir, 'video'));
    writeFileSync(join(dir, 'video', 'library.json'), JSON.stringify({ package: 'expo-video' }));

    expect([...bundledImplementations(dir).keys()]).toEqual(['expo-video']);
  });
});
