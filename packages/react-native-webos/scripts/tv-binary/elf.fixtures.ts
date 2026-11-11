interface FixtureImport {
  readonly name: string;
  readonly weak?: boolean;
  /** The library and version node the import is bound to. */
  readonly version?: { readonly library: string; readonly node: string };
}

interface FixtureElf {
  readonly interpreter: string;
  readonly needed: readonly string[];
  readonly imports: readonly FixtureImport[];
  readonly flags: number;
  /** Extra sections, by name, type and content. */
  readonly sections?: readonly { name: string; type: number; bytes: Uint8Array }[];
}

class StringTable {
  private readonly parts: string[] = [''];
  private length = 1;

  add(text: string): number {
    const offset = this.length;
    this.parts.push(text);
    this.length += text.length + 1;
    return offset;
  }

  bytes(): Uint8Array {
    return new TextEncoder().encode(`${this.parts.join('\0')}\0`);
  }
}

function words(values: readonly number[], size: 2 | 4 = 4): Uint8Array {
  const bytes = new Uint8Array(values.length * size);
  const view = new DataView(bytes.buffer);
  values.forEach((value, index) => {
    if (size === 2) view.setUint16(index * 2, value, true);
    else view.setUint32(index * 4, value, true);
  });
  return bytes;
}

function concat(parts: readonly Uint8Array[]): Uint8Array {
  const bytes = new Uint8Array(parts.reduce((total, part) => total + part.length, 0));
  let at = 0;
  for (const part of parts) {
    bytes.set(part, at);
    at += part.length;
  }
  return bytes;
}

/** A little-endian ELF32 ARM executable holding just what `parseElf` reads. */
export function buildElf(fixture: FixtureElf): Uint8Array {
  const dynstr = new StringTable();
  const nodes = new Map<string, { library: string; node: string; index: number }>();
  for (const { version } of fixture.imports) {
    if (version === undefined) continue;
    const key = `${version.library}\0${version.node}`;
    if (!nodes.has(key)) nodes.set(key, { ...version, index: nodes.size + 2 });
  }
  const symbols: number[] = [0, 0, 0, 0];
  const versym: number[] = [0];
  for (const { name, weak, version } of fixture.imports) {
    symbols.push(dynstr.add(name), 0, 0, (weak ? 2 : 1) << 4);
    versym.push(
      version === undefined ? 1 : (nodes.get(`${version.library}\0${version.node}`)?.index ?? 1),
    );
  }
  const verneed: number[] = [];
  const libraries = [...new Set([...nodes.values()].map((node) => node.library))];
  libraries.forEach((library, libraryIndex) => {
    const own = [...nodes.values()].filter((node) => node.library === library);
    const last = libraryIndex === libraries.length - 1;
    verneed.push(1 | (own.length << 16), dynstr.add(library), 16, last ? 0 : 16 + own.length * 16);
    own.forEach((node, index) => {
      verneed.push(0, node.index << 16, dynstr.add(node.node), index === own.length - 1 ? 0 : 16);
    });
  });
  const dynamic = fixture.needed.flatMap((name) => [1, dynstr.add(name)]).concat([0, 0]);

  const shstr = new StringTable();
  const sections = [
    { name: '', type: 0, bytes: new Uint8Array(), link: 0, info: 0, entry: 0 },
    {
      name: '.interp',
      type: 1,
      bytes: new TextEncoder().encode(`${fixture.interpreter}\0`),
      link: 0,
      info: 0,
      entry: 0,
    },
    { name: '.dynsym', type: 11, bytes: words(symbols), link: 3, info: 1, entry: 16 },
    { name: '.dynstr', type: 3, bytes: dynstr.bytes(), link: 0, info: 0, entry: 0 },
    { name: '.gnu.version', type: 0x6fffffff, bytes: words(versym, 2), link: 2, info: 0, entry: 2 },
    {
      name: '.gnu.version_r',
      type: 0x6ffffffe,
      bytes: words(verneed),
      link: 3,
      info: libraries.length,
      entry: 0,
    },
    { name: '.dynamic', type: 6, bytes: words(dynamic), link: 3, info: 0, entry: 8 },
    ...(fixture.sections ?? []).map((section) => ({ ...section, link: 0, info: 0, entry: 0 })),
  ];
  const named = sections.map((section) => ({ ...section, nameOffset: shstr.add(section.name) }));
  const shstrIndex = named.length;
  named.push({
    name: '.shstrtab',
    type: 3,
    bytes: new Uint8Array(),
    link: 0,
    info: 0,
    entry: 0,
    nameOffset: shstr.add('.shstrtab'),
  });
  const last = named[shstrIndex];
  if (last !== undefined) last.bytes = shstr.bytes();

  const headerSize = 52;
  const programHeaderSize = 32;
  let offset = headerSize + programHeaderSize;
  const placed = named.map((section) => {
    const at = offset;
    offset += section.bytes.length;
    return { ...section, offset: at };
  });
  const sectionTable = offset;
  const header = new Uint8Array(headerSize);
  header.set([0x7f, 0x45, 0x4c, 0x46, 1, 1, 1]);
  const view = new DataView(header.buffer);
  view.setUint16(0x10, 2, true);
  view.setUint16(0x12, 40, true);
  view.setUint32(0x1c, headerSize, true);
  view.setUint32(0x20, sectionTable, true);
  view.setUint32(0x24, fixture.flags, true);
  view.setUint16(0x2a, programHeaderSize, true);
  view.setUint16(0x2c, 1, true);
  view.setUint16(0x2e, 40, true);
  view.setUint16(0x30, placed.length, true);
  view.setUint16(0x32, shstrIndex, true);
  const interp = placed[1];
  const programHeader = words([3, interp?.offset ?? 0, 0, 0, interp?.bytes.length ?? 0, 0, 4, 1]);
  const table = placed.flatMap((section) => [
    section.nameOffset,
    section.type,
    0,
    0,
    section.offset,
    section.bytes.length,
    section.link,
    section.info,
    1,
    section.entry,
  ]);
  return concat([header, programHeader, ...placed.map((section) => section.bytes), words(table)]);
}
