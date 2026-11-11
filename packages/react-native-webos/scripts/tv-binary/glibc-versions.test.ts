import { describe, expect, it } from 'vitest';
import { compareVersionNodes, highestVersion, versionUses } from './glibc-versions.ts';

const imports = [
  { name: 'memcpy', version: 'GLIBC_2.4', library: 'libc.so.6', weak: false },
  { name: 'pthread_setname_np', version: 'GLIBC_2.12', library: 'libpthread.so.0', weak: false },
  { name: 'pipe2', version: 'GLIBC_2.9', library: 'libc.so.6', weak: false },
  { name: 'strlen', version: 'GLIBC_2.4', library: 'libc.so.6', weak: false },
  { name: 'eglSwapBuffers', version: null, library: null, weak: false },
];

describe('versionUses', () => {
  it('groups imports by version node, highest last', () => {
    expect(versionUses(imports)).toEqual([
      { library: 'libc.so.6', version: 'GLIBC_2.4', symbols: ['memcpy', 'strlen'] },
      { library: 'libc.so.6', version: 'GLIBC_2.9', symbols: ['pipe2'] },
      { library: 'libpthread.so.0', version: 'GLIBC_2.12', symbols: ['pthread_setname_np'] },
    ]);
  });

  it('finds the highest node of a family', () => {
    const uses = versionUses(imports);
    expect(highestVersion(uses, 'GLIBC_')).toBe('GLIBC_2.12');
    expect(highestVersion(uses, 'GLIBCXX_')).toBeNull();
  });
});

describe('compareVersionNodes', () => {
  it('compares each number, not the text', () => {
    expect(compareVersionNodes('GLIBC_2.9', 'GLIBC_2.12')).toBeLessThan(0);
    expect(compareVersionNodes('GLIBCXX_3.4.30', 'GLIBCXX_3.4.9')).toBeGreaterThan(0);
    expect(compareVersionNodes('GLIBC_2.4', 'GLIBC_2.4')).toBe(0);
  });
});
