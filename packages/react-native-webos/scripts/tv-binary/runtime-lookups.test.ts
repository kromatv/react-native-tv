import { describe, expect, it } from 'vitest';
import { parseElf } from './elf-file.ts';
import { buildElf } from './elf.fixtures.ts';
import { firmware } from './firmware.fixtures.ts';
import { literalStrings, runtimeLibraries } from './runtime-lookups.ts';

const strings = [
  'libwayland-client.so.0',
  'libdecor-0.so.0',
  'wl_proxy_marshal',
  'wl_proxy_set_tag',
  'move',
];
const elf = parseElf(
  buildElf({
    interpreter: '/lib/ld-linux.so.3',
    needed: ['libc.so.6'],
    flags: 0,
    imports: [],
    sections: [
      {
        name: '.rodata',
        type: 1,
        bytes: new TextEncoder().encode(`${strings.join('\0')}\0\x01ab\0`),
      },
    ],
  }),
);
const firmwares = [
  firmware('4.4.2', {
    'libwayland-client.so.0': { needed: ['libffi.so.6'], symbols: ['wl_proxy_marshal'] },
    'libffi.so.6': { symbols: ['move'] },
  }),
  firmware('9.2.0', {
    'libwayland-client.so.0': { symbols: ['wl_proxy_marshal', 'wl_proxy_set_tag'] },
  }),
];

describe('literalStrings', () => {
  it('collects NUL-terminated printable strings of three characters or more', () => {
    expect([...literalStrings(elf)].sort()).toEqual([...strings].sort());
  });
});

describe('runtimeLibraries', () => {
  it('lists every library name among the strings, and where it is absent', () => {
    const libraries = runtimeLibraries(elf, firmwares, []);
    expect(libraries.map(({ name, absentOn }) => ({ name, absentOn }))).toEqual([
      { name: 'libdecor-0.so.0', absentOn: ['4.4.2', '9.2.0'] },
      { name: 'libwayland-client.so.0', absentOn: [] },
    ]);
    expect(libraries.every((library) => library.lookups.size === 0)).toBe(true);
  });

  it('reports the symbols of a required library that some image lacks', () => {
    const [wayland] = runtimeLibraries(elf, firmwares, ['libwayland-client.so.0']).filter(
      (library) => library.required,
    );
    expect(wayland?.lookups).toEqual(new Map([['wl_proxy_set_tag', ['4.4.2']]]));
  });

  it('adds a required library no string names', () => {
    const libraries = runtimeLibraries(elf, firmwares, ['libhelpers.so.2']);
    expect(libraries.find((library) => library.name === 'libhelpers.so.2')?.absentOn).toEqual([
      '4.4.2',
      '9.2.0',
    ]);
  });
});
