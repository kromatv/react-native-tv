import { describe, expect, it } from 'vitest';
import { checkFirmware, compatibilityFloor, isCompatible } from './firmware-check.ts';
import { firmware } from './firmware.fixtures.ts';

const libc = {
  symbols: ['GLIBC_2.4@GLIBC_2.4', 'memcpy@GLIBC_2.4', 'GLIBC_2.9@GLIBC_2.9', 'pipe2@GLIBC_2.9'],
};
const strong = (name: string, version: string | null = null, library: string | null = null) => ({
  name,
  version,
  library,
  weak: false,
});
const needs = {
  needed: ['libEGL.so.1', 'libc.so.6'],
  opened: ['libwayland-client.so.0'],
  imports: [
    strong('memcpy', 'GLIBC_2.4', 'libc.so.6'),
    strong('pipe2', 'GLIBC_2.9', 'libc.so.6'),
    strong('eglSwapBuffers'),
    { name: '__gmon_start__', version: null, library: null, weak: true },
  ],
};
const complete = {
  'libc.so.6': libc,
  'libEGL.so.1': { needed: ['libmali.so'], symbols: [] },
  'libmali.so': { symbols: ['eglSwapBuffers'] },
  'libwayland-client.so.0': { symbols: ['wl_display_connect'] },
};

describe('checkFirmware', () => {
  it('finds a symbol in a library the needed one depends on', () => {
    const result = checkFirmware(firmware('5.3.1', complete), needs);
    expect(isCompatible(result)).toBe(true);
    expect(result.missingWeakSymbols).toEqual(['__gmon_start__']);
  });

  it('reports a needed and an opened library the image lacks', () => {
    const { 'libEGL.so.1': _egl, 'libwayland-client.so.0': _wayland, ...rest } = complete;
    const result = checkFirmware(firmware('1.2.0', rest), needs);
    expect(result.missingLibraries).toEqual(['libEGL.so.1', 'libwayland-client.so.0']);
    expect(result.missingSymbols).toEqual(['eglSwapBuffers']);
    expect(isCompatible(result)).toBe(false);
  });

  it('reports a version node and the symbol under it', () => {
    const old = {
      ...complete,
      'libc.so.6': { symbols: ['GLIBC_2.4@GLIBC_2.4', 'memcpy@GLIBC_2.4'] },
    };
    const result = checkFirmware(firmware('1.2.0', old), needs);
    expect(result.missingVersions).toEqual(['libc.so.6: GLIBC_2.9']);
    expect(result.missingSymbols).toEqual(['pipe2@GLIBC_2.9']);
  });

  it('does not take another version of a symbol for the one asked', () => {
    const moved = {
      ...complete,
      'libc.so.6': { symbols: [...libc.symbols.slice(0, 3), 'pipe2@GLIBC_2.17'] },
    };
    expect(checkFirmware(firmware('7.4.0', moved), needs).missingSymbols).toEqual([
      'pipe2@GLIBC_2.9',
    ]);
  });

  it('never looks for a symbol in a library that is only opened', () => {
    const elsewhere = {
      ...complete,
      'libmali.so': { symbols: [] },
      'libwayland-client.so.0': { symbols: ['eglSwapBuffers'] },
    };
    expect(checkFirmware(firmware('5.3.1', elsewhere), needs).missingSymbols).toEqual([
      'eglSwapBuffers',
    ]);
  });
});

describe('compatibilityFloor', () => {
  const result = (release: string, missing: readonly string[]) => ({
    release,
    otaId: '',
    missingLibraries: missing,
    missingVersions: [],
    missingSymbols: [],
    missingWeakSymbols: [],
  });

  it('is the oldest release from which every image passes', () => {
    const results = [result('1.2.0', ['libatomic.so.1']), result('3.4.0', []), result('4.4.2', [])];
    expect(compatibilityFloor(results)).toBe('3.4.0');
  });

  it('starts again after an image that fails between two that pass', () => {
    const results = [result('3.9.2', []), result('4.4.2', ['libatomic.so.1']), result('5.3.1', [])];
    expect(compatibilityFloor(results)).toBe('5.3.1');
  });

  it('is null when the newest image fails', () => {
    expect(
      compatibilityFloor([result('10.2.0', []), result('11.2.0', ['libcurl.so.5'])]),
    ).toBeNull();
  });
});
