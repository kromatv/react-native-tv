const LOCAL = 0;
const GLOBAL = 1;

/** Symbol bindings, as an ELF symbol's info byte holds them in its high nibble. */
export const BINDING = { LOCAL, GLOBAL, WEAK: 2 } as const;

/**
 * A little-endian ELF shared object holding one `.dynsym` table with a symbol of
 * each of `bindings`, the global part starting at `firstGlobal`.
 */
export function sharedObject(
  is64: boolean,
  bindings: readonly number[],
  firstGlobal: number,
): Uint8Array {
  const headerSize = is64 ? 64 : 52;
  const sectionHeaderSize = is64 ? 64 : 40;
  const symbolSize = is64 ? 24 : 16;
  const symbolsSize = bindings.length * symbolSize;
  const sectionTable = headerSize + symbolsSize;
  const elf = new Uint8Array(sectionTable + 2 * sectionHeaderSize);
  const view = new DataView(elf.buffer);
  elf.set([0x7f, 0x45, 0x4c, 0x46, is64 ? 2 : 1, 1]);
  view.setUint16(0x10, 3, true);
  bindings.forEach((binding, index) => {
    elf[headerSize + index * symbolSize + (is64 ? 4 : 12)] = (binding << 4) | 2;
  });
  const dynsym = sectionTable + sectionHeaderSize;
  view.setUint32(dynsym + 4, 11, true);
  if (is64) {
    view.setBigUint64(0x28, BigInt(sectionTable), true);
    view.setUint16(0x3a, sectionHeaderSize, true);
    view.setUint16(0x3c, 2, true);
    view.setBigUint64(dynsym + 0x18, BigInt(headerSize), true);
    view.setBigUint64(dynsym + 0x20, BigInt(symbolsSize), true);
    view.setUint32(dynsym + 0x2c, firstGlobal, true);
    view.setBigUint64(dynsym + 0x38, BigInt(symbolSize), true);
  } else {
    view.setUint32(0x20, sectionTable, true);
    view.setUint16(0x2e, sectionHeaderSize, true);
    view.setUint16(0x30, 2, true);
    view.setUint32(dynsym + 0x10, headerSize, true);
    view.setUint32(dynsym + 0x14, symbolsSize, true);
    view.setUint32(dynsym + 0x1c, firstGlobal, true);
    view.setUint32(dynsym + 0x24, symbolSize, true);
  }
  return elf;
}

/** The binding of each symbol in a {@link sharedObject}. */
export function bindings(elf: Uint8Array, is64: boolean, count: number): number[] {
  const headerSize = is64 ? 64 : 52;
  const symbolSize = is64 ? 24 : 16;
  return Array.from(
    { length: count },
    (_, index) => (elf[headerSize + index * symbolSize + (is64 ? 4 : 12)] ?? 0) >> 4,
  );
}
