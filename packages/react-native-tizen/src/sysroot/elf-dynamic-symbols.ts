const ELF_MAGIC = [0x7f, 0x45, 0x4c, 0x46] as const;
const ELF_CLASS_32 = 1;
const ELF_CLASS_64 = 2;
const ELF_DATA_LITTLE_ENDIAN = 1;
const ELF_TYPE_SHARED_OBJECT = 3;
const SECTION_TYPE_DYNSYM = 11;
const BINDING_LOCAL = 0;
const BINDING_WEAK = 2;

interface ElfLayout {
  readonly is64: boolean;
  readonly view: DataView;
}

interface DynamicSymbolTable {
  readonly offset: number;
  readonly size: number;
  readonly firstGlobal: number;
  readonly entrySize: number;
}

function readLayout(elf: Uint8Array): ElfLayout | null {
  if (elf.length < 64 || ELF_MAGIC.some((byte, index) => elf[index] !== byte)) return null;
  const elfClass = elf[4];
  if (elfClass !== ELF_CLASS_32 && elfClass !== ELF_CLASS_64) return null;
  if (elf[5] !== ELF_DATA_LITTLE_ENDIAN) return null;
  const view = new DataView(elf.buffer, elf.byteOffset, elf.byteLength);
  return { is64: elfClass === ELF_CLASS_64, view };
}

function readSectionTableOffset({ is64, view }: ElfLayout): number {
  return is64 ? Number(view.getBigUint64(0x28, true)) : view.getUint32(0x20, true);
}

function readDynamicSymbolTables(layout: ElfLayout): DynamicSymbolTable[] {
  const { is64, view } = layout;
  const tableOffset = readSectionTableOffset(layout);
  const headerSize = view.getUint16(is64 ? 0x3a : 0x2e, true);
  const headerCount = view.getUint16(is64 ? 0x3c : 0x30, true);
  const tables: DynamicSymbolTable[] = [];
  for (let index = 0; index < headerCount; index += 1) {
    const header = tableOffset + index * headerSize;
    if (header + headerSize > view.byteLength) break;
    if (view.getUint32(header + 4, true) !== SECTION_TYPE_DYNSYM) continue;
    tables.push(
      is64
        ? {
            offset: Number(view.getBigUint64(header + 0x18, true)),
            size: Number(view.getBigUint64(header + 0x20, true)),
            firstGlobal: view.getUint32(header + 0x2c, true),
            entrySize: Number(view.getBigUint64(header + 0x38, true)),
          }
        : {
            offset: view.getUint32(header + 0x10, true),
            size: view.getUint32(header + 0x14, true),
            firstGlobal: view.getUint32(header + 0x1c, true),
            entrySize: view.getUint32(header + 0x24, true),
          },
    );
  }
  return tables;
}

/** True for a little-endian ELF shared object, the only kind {@link rebindStrayLocalSymbols} edits. */
export function isElfSharedObject(elf: Uint8Array): boolean {
  const layout = readLayout(elf);
  return layout !== null && layout.view.getUint16(0x10, true) === ELF_TYPE_SHARED_OBJECT;
}

/**
 * Rebinds, in place, every LOCAL symbol a Tizen rootstrap leaves in the global
 * part of a `.dynsym` table. The rootstrap's stub libraries neuter their imports
 * that way; GNU ld skips them, lld refuses the whole library. The entries become
 * WEAK, which neither linker resolves anything against. Returns how many changed.
 */
export function rebindStrayLocalSymbols(elf: Uint8Array): number {
  const layout = readLayout(elf);
  if (layout === null) return 0;
  const infoField = layout.is64 ? 4 : 12;
  let rebound = 0;
  for (const table of readDynamicSymbolTables(layout)) {
    if (table.entrySize === 0) continue;
    const entryCount = Math.floor(table.size / table.entrySize);
    for (let entry = table.firstGlobal; entry < entryCount; entry += 1) {
      const infoOffset = table.offset + entry * table.entrySize + infoField;
      if (infoOffset >= elf.length) break;
      const info = elf[infoOffset] ?? 0;
      if (info >> 4 !== BINDING_LOCAL) continue;
      elf[infoOffset] = (BINDING_WEAK << 4) | (info & 0x0f);
      rebound += 1;
    }
  }
  return rebound;
}
