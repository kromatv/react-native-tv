import { FixableError } from '../../src/errors.ts';

const SECTION_DYNAMIC = 6;
const SECTION_DYNSYM = 11;
const SECTION_VERSYM = 0x6fffffff;
const SECTION_VERNEED = 0x6ffffffe;
const SEGMENT_INTERP = 3;
const DYNAMIC_NEEDED = 1;
const BINDING_WEAK = 2;
const MAX_STRING = 4096;

/** One section of an ELF file. */
export interface ElfSection {
  readonly name: string;
  readonly type: number;
  readonly offset: number;
  readonly size: number;
  readonly link: number;
  readonly info: number;
  readonly entrySize: number;
}

/** A symbol the file leaves for the dynamic linker to find in another library. */
export interface ImportedSymbol {
  readonly name: string;
  /** The version node it must come from, such as `GLIBC_2.4`; null for an unversioned reference. */
  readonly version: string | null;
  /** The library that version node belongs to, as the file records it. */
  readonly library: string | null;
  /** A weak reference resolves to null when nothing defines it, so its absence is survivable. */
  readonly weak: boolean;
}

/** What the dynamic linker reads of a little-endian ELF executable or library. */
export interface ElfFile {
  readonly is64: boolean;
  /** `e_type`: 2 is an executable at a fixed address, 3 a shared object or a position-independent executable. */
  readonly type: number;
  readonly machine: number;
  readonly flags: number;
  readonly interpreter: string | null;
  readonly needed: readonly string[];
  readonly imports: readonly ImportedSymbol[];
  readonly sections: readonly ElfSection[];
  /** The bytes of a section; empty for one that occupies no file space. */
  sectionBytes(section: ElfSection): Uint8Array;
}

class Reader {
  readonly view: DataView;

  constructor(
    readonly bytes: Uint8Array,
    readonly is64: boolean,
  ) {
    this.view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  }

  u16(offset: number): number {
    return this.view.getUint16(offset, true);
  }

  u32(offset: number): number {
    return this.view.getUint32(offset, true);
  }

  /** An address-sized field: 4 bytes in ELF32, 8 in ELF64. */
  word(offset: number): number {
    return this.is64 ? Number(this.view.getBigUint64(offset, true)) : this.u32(offset);
  }

  string(offset: number): string {
    let end = offset;
    while (end < this.bytes.length && end - offset < MAX_STRING && this.bytes[end] !== 0) end += 1;
    return new TextDecoder().decode(this.bytes.subarray(offset, end));
  }
}

function readSections(reader: Reader): ElfSection[] {
  const { is64 } = reader;
  const tableOffset = reader.word(is64 ? 0x28 : 0x20);
  const entrySize = reader.u16(is64 ? 0x3a : 0x2e);
  const count = reader.u16(is64 ? 0x3c : 0x30);
  const namesIndex = reader.u16(is64 ? 0x3e : 0x32);
  const raw = Array.from({ length: count }, (_, index) => {
    const at = tableOffset + index * entrySize;
    return {
      nameOffset: reader.u32(at),
      type: reader.u32(at + 4),
      offset: reader.word(at + (is64 ? 0x18 : 0x10)),
      size: reader.word(at + (is64 ? 0x20 : 0x14)),
      link: reader.u32(at + (is64 ? 0x28 : 0x18)),
      info: reader.u32(at + (is64 ? 0x2c : 0x1c)),
      entrySize: reader.word(at + (is64 ? 0x38 : 0x24)),
    };
  });
  const names = raw[namesIndex];
  return raw.map(({ nameOffset, ...section }) => ({
    ...section,
    name: names === undefined ? '' : reader.string(names.offset + nameOffset),
  }));
}

function readInterpreter(reader: Reader): string | null {
  const { is64 } = reader;
  const tableOffset = reader.word(is64 ? 0x20 : 0x1c);
  const entrySize = reader.u16(is64 ? 0x36 : 0x2a);
  const count = reader.u16(is64 ? 0x38 : 0x2c);
  for (let index = 0; index < count; index += 1) {
    const at = tableOffset + index * entrySize;
    if (reader.u32(at) === SEGMENT_INTERP) return reader.string(reader.word(at + (is64 ? 8 : 4)));
  }
  return null;
}

function readNeeded(reader: Reader, sections: readonly ElfSection[]): string[] {
  const dynamic = sections.find((section) => section.type === SECTION_DYNAMIC);
  const strings = dynamic === undefined ? undefined : sections[dynamic.link];
  if (dynamic === undefined || strings === undefined) return [];
  const entrySize = reader.is64 ? 16 : 8;
  const needed: string[] = [];
  for (let at = dynamic.offset; at + entrySize <= dynamic.offset + dynamic.size; at += entrySize) {
    const tag = reader.word(at);
    if (tag === 0) break;
    if (tag === DYNAMIC_NEEDED) {
      needed.push(reader.string(strings.offset + reader.word(at + entrySize / 2)));
    }
  }
  return needed;
}

interface VersionNode {
  readonly version: string;
  readonly library: string;
}

/** The version nodes `.gnu.version_r` requires, by the index `.gnu.version` gives a symbol. */
function readVersionNodes(
  reader: Reader,
  sections: readonly ElfSection[],
): Map<number, VersionNode> {
  const nodes = new Map<number, VersionNode>();
  const verneed = sections.find((section) => section.type === SECTION_VERNEED);
  const strings = verneed === undefined ? undefined : sections[verneed.link];
  if (verneed === undefined || strings === undefined) return nodes;
  let entry = verneed.offset;
  for (let index = 0; index < verneed.info; index += 1) {
    const library = reader.string(strings.offset + reader.u32(entry + 4));
    let aux = entry + reader.u32(entry + 8);
    for (let count = reader.u16(entry + 2); count > 0; count -= 1) {
      const version = reader.string(strings.offset + reader.u32(aux + 8));
      nodes.set(reader.u16(aux + 6) & 0x7fff, { version, library });
      aux += reader.u32(aux + 12);
    }
    entry += reader.u32(entry + 12);
  }
  return nodes;
}

function readImports(reader: Reader, sections: readonly ElfSection[]): ImportedSymbol[] {
  const dynsym = sections.find((section) => section.type === SECTION_DYNSYM);
  const strings = dynsym === undefined ? undefined : sections[dynsym.link];
  if (dynsym === undefined || strings === undefined || dynsym.entrySize === 0) return [];
  const versym = sections.find((section) => section.type === SECTION_VERSYM);
  const nodes = readVersionNodes(reader, sections);
  const imports: ImportedSymbol[] = [];
  const count = Math.floor(dynsym.size / dynsym.entrySize);
  for (let index = 1; index < count; index += 1) {
    const at = dynsym.offset + index * dynsym.entrySize;
    const sectionIndex = reader.u16(at + (reader.is64 ? 6 : 14));
    if (sectionIndex !== 0) continue;
    const info = reader.bytes[at + (reader.is64 ? 4 : 12)] ?? 0;
    const node =
      versym === undefined ? undefined : nodes.get(reader.u16(versym.offset + index * 2) & 0x7fff);
    imports.push({
      name: reader.string(strings.offset + reader.u32(at)),
      version: node?.version ?? null,
      library: node?.library ?? null,
      weak: info >> 4 === BINDING_WEAK,
    });
  }
  return imports;
}

/** Reads the parts of an ELF file that decide whether a TV's dynamic linker can start it. */
export function parseElf(bytes: Uint8Array): ElfFile {
  const magic = bytes[0] === 0x7f && bytes[1] === 0x45 && bytes[2] === 0x4c && bytes[3] === 0x46;
  const elfClass = bytes[4];
  if (bytes.length < 64 || !magic || (elfClass !== 1 && elfClass !== 2) || bytes[5] !== 1) {
    throw new FixableError(
      'Not a little-endian ELF file',
      'Pass the executable a TV build produced, such as <build>/host/react-native-webos.',
    );
  }
  const reader = new Reader(bytes, elfClass === 2);
  try {
    const sections = readSections(reader);
    return {
      is64: reader.is64,
      type: reader.u16(0x10),
      machine: reader.u16(0x12),
      flags: reader.u32(reader.is64 ? 0x30 : 0x24),
      interpreter: readInterpreter(reader),
      needed: readNeeded(reader, sections),
      imports: readImports(reader, sections),
      sections,
      sectionBytes: (section) =>
        section.type === 8
          ? new Uint8Array()
          : bytes.subarray(section.offset, section.offset + section.size),
    };
  } catch (error) {
    if (!(error instanceof RangeError)) throw error;
    throw new FixableError('The ELF file is truncated or corrupt', 'Build the runtime again.');
  }
}
