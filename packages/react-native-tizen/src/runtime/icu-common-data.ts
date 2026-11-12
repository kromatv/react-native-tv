/** One item of an ICU common data file, named as inside it without the package prefix. */
export interface IcuDataItem {
  readonly name: string;
  readonly bytes: Uint8Array;
}

/** An ICU common data file (`icudt<major>l.dat`): its header and every item, in name order. */
export interface IcuCommonData {
  readonly header: Uint8Array;
  readonly packageName: string;
  readonly items: readonly IcuDataItem[];
}

const MAGIC = [0xda, 0x27];
const COMMON_FORMAT = 'CmnD';
const RESOURCE_FORMAT = 'ResB';
const ALIGNMENT = 16;
const TABLE = 2;
const TABLE16 = 5;

function view(bytes: Uint8Array): DataView {
  return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
}

function ascii(bytes: Uint8Array, start: number, length: number): string {
  return String.fromCodePoint(...bytes.subarray(start, start + length));
}

function cString(bytes: Uint8Array, start: number): string {
  const end = bytes.indexOf(0, start);
  if (end < 0) throw new Error('An ICU data string has no terminator');
  return ascii(bytes, start, end - start);
}

function dataHeaderSize(bytes: Uint8Array, format: string): number {
  const header = view(bytes);
  if (bytes[2] !== MAGIC[0] || bytes[3] !== MAGIC[1]) throw new Error('Not an ICU data file');
  if (bytes[8] !== 0) throw new Error('Only little-endian ICU data is supported');
  if (ascii(bytes, 12, 4) !== format) throw new Error(`Not ICU ${format} data`);
  return header.getUint16(0, true);
}

/** Reads a common data file into its items, which keep their own bytes. */
export function readIcuCommonData(file: Uint8Array): IcuCommonData {
  const headerSize = dataHeaderSize(file, COMMON_FORMAT);
  const toc = view(file.subarray(headerSize));
  const count = toc.getUint32(0, true);
  const entries = Array.from({ length: count }, (_, index) => ({
    name: cString(file, headerSize + toc.getUint32(4 + index * 8, true)),
    start: headerSize + toc.getUint32(8 + index * 8, true),
  }));
  const ends = [...entries.slice(1).map((entry) => entry.start), file.byteLength];
  const [first] = entries;
  const slash = first?.name.indexOf('/') ?? -1;
  if (first === undefined || slash < 0) throw new Error('An ICU data file without items');
  const packageName = first.name.slice(0, slash);
  return {
    header: file.slice(0, headerSize),
    packageName,
    items: entries.map((entry, index) => ({
      name: entry.name.slice(packageName.length + 1),
      bytes: file.slice(entry.start, ends[index]),
    })),
  };
}

const align = (offset: number) => Math.ceil(offset / ALIGNMENT) * ALIGNMENT;

/** Writes `data` back as one common data file, every item aligned as ICU's own tools align it. */
export function writeIcuCommonData(data: IcuCommonData): Uint8Array {
  const names = data.items.map((item) =>
    new TextEncoder().encode(`${data.packageName}/${item.name}\0`),
  );
  const tocSize = 4 + data.items.length * 8;
  const namesSize = names.reduce((total, name) => total + name.byteLength, 0);
  let offset = align(data.header.byteLength + tocSize + namesSize);
  const starts = data.items.map((item) => {
    const start = offset;
    offset = align(start + item.bytes.byteLength);
    return start;
  });
  const file = new Uint8Array(offset);
  file.set(data.header, 0);
  const toc = view(file.subarray(data.header.byteLength));
  toc.setUint32(0, data.items.length, true);
  let nameOffset = tocSize;
  data.items.forEach((item, index) => {
    toc.setUint32(4 + index * 8, nameOffset, true);
    toc.setUint32(8 + index * 8, (starts[index] ?? 0) - data.header.byteLength, true);
    file.set(names[index] ?? new Uint8Array(), data.header.byteLength + nameOffset);
    nameOffset += names[index]?.byteLength ?? 0;
    file.set(item.bytes, starts[index] ?? 0);
  });
  return file;
}

function rootTableEntry(bytes: Uint8Array, rootStart: number, key: string): number {
  const root = view(bytes.subarray(rootStart));
  const resource = root.getUint32(0, true);
  if (resource >>> 28 !== TABLE) throw new Error('res_index.res has no root table');
  const table = (resource & 0x0fffffff) * 4;
  const count = root.getUint16(table, true);
  const items = table + 2 + count * 2 + ((count * 2 + 2) % 4);
  for (let index = 0; index < count; index += 1) {
    if (cString(bytes, rootStart + root.getUint16(table + 2 + index * 2, true)) === key) {
      return root.getUint32(items + index * 4, true);
    }
  }
  throw new Error(`res_index.res has no ${key}`);
}

/**
 * A copy of a tree's `res_index.res` whose `InstalledLocales` lists only the
 * locales `keep` accepts, so ICU offers no locale whose data was left out.
 * The table shrinks in place; nothing else in the bundle moves.
 */
export function filterInstalledLocales(
  resIndex: Uint8Array,
  keep: (locale: string) => boolean,
): Uint8Array {
  const bytes = resIndex.slice();
  const rootStart = dataHeaderSize(bytes, RESOURCE_FORMAT);
  const body = view(bytes.subarray(rootStart));
  const resource = rootTableEntry(bytes, rootStart, 'InstalledLocales');
  if (resource >>> 28 !== TABLE16) throw new Error('InstalledLocales is not a 16-bit table');
  const keysTop = body.getUint32(4 + 4, true) * 4;
  const table = keysTop + (resource & 0x0fffffff) * 2;
  const count = body.getUint16(table, true);
  const kept: [number, number][] = [];
  for (let index = 0; index < count; index += 1) {
    const keyOffset = body.getUint16(table + 2 + index * 2, true);
    const value = body.getUint16(table + 2 + (count + index) * 2, true);
    if (keep(cString(bytes, rootStart + keyOffset))) kept.push([keyOffset, value]);
  }
  body.setUint16(table, kept.length, true);
  kept.forEach(([keyOffset, value], index) => {
    body.setUint16(table + 2 + index * 2, keyOffset, true);
    body.setUint16(table + 2 + (kept.length + index) * 2, value, true);
  });
  return bytes;
}

/** The locales a tree's `res_index.res` lists, in its order. */
export function installedLocales(resIndex: Uint8Array): string[] {
  const rootStart = dataHeaderSize(resIndex, RESOURCE_FORMAT);
  const body = view(resIndex.subarray(rootStart));
  const resource = rootTableEntry(resIndex, rootStart, 'InstalledLocales');
  const table = body.getUint32(8, true) * 4 + (resource & 0x0fffffff) * 2;
  const count = body.getUint16(table, true);
  return Array.from({ length: count }, (_, index) =>
    cString(resIndex, rootStart + body.getUint16(table + 2 + index * 2, true)),
  );
}
