import { inflateRawSync } from 'node:zlib';

/** A file or a directory in a zip archive, its bytes inflated. */
export interface ZipEntry {
  readonly name: string;
  readonly directory: boolean;
  readonly data: Buffer;
}

const END_SIGNATURE = 0x06054b50;
const CENTRAL_SIGNATURE = 0x02014b50;
const LOCAL_SIGNATURE = 0x04034b50;
const END_BYTES = 22;
const MAX_COMMENT = 0xffff;
const MAX_TOTAL_BYTES = 1024 * 1024 * 1024;
const UNSAFE_SEGMENT = new Set(['', '.', '..']);

function endRecord(zip: Buffer): number {
  const lowest = Math.max(0, zip.length - END_BYTES - MAX_COMMENT);
  for (let at = zip.length - END_BYTES; at >= lowest; at--) {
    if (zip.readUInt32LE(at) === END_SIGNATURE) return at;
  }
  throw new Error('zip: no end of central directory, so this is not a zip file');
}

function checkedName(name: string): string {
  const path = name.endsWith('/') ? name.slice(0, -1) : name;
  if (
    path.startsWith('/') ||
    path.includes('\\') ||
    path.split('/').some((segment) => UNSAFE_SEGMENT.has(segment))
  ) {
    throw new Error(`zip: refusing the entry name ${JSON.stringify(name)}`);
  }
  return path;
}

function entryData(
  zip: Buffer,
  offset: number,
  method: number,
  compressed: number,
  size: number,
): Buffer {
  if (zip.readUInt32LE(offset) !== LOCAL_SIGNATURE) throw new Error('zip: a broken local header');
  const start = offset + 30 + zip.readUInt16LE(offset + 26) + zip.readUInt16LE(offset + 28);
  const raw = zip.subarray(start, start + compressed);
  if (raw.length !== compressed) throw new Error('zip: an entry runs past the end of the file');
  if (method === 0) return raw;
  if (method !== 8) throw new Error(`zip: unsupported compression method ${method}`);
  const data = inflateRawSync(raw, { maxOutputLength: Math.max(size, 1) });
  if (data.length !== size) throw new Error('zip: an entry inflated to the wrong size');
  return data;
}

/** Reads every entry of a zip archive. Refuses ZIP64, encryption and paths that leave the archive. */
export function readZip(zip: Buffer): ZipEntry[] {
  const end = endRecord(zip);
  const count = zip.readUInt16LE(end + 10);
  let at = zip.readUInt32LE(end + 16);
  if (at === 0xffffffff || count === 0xffff)
    throw new Error('zip: ZIP64 archives are not supported');
  const entries: ZipEntry[] = [];
  let total = 0;
  for (let i = 0; i < count; i++) {
    if (zip.readUInt32LE(at) !== CENTRAL_SIGNATURE)
      throw new Error('zip: a broken central directory');
    const flags = zip.readUInt16LE(at + 8);
    const method = zip.readUInt16LE(at + 10);
    const compressed = zip.readUInt32LE(at + 20);
    const size = zip.readUInt32LE(at + 24);
    const nameLength = zip.readUInt16LE(at + 28);
    const name = zip.subarray(at + 46, at + 46 + nameLength).toString('utf8');
    if (flags & 1) throw new Error(`zip: ${name} is encrypted`);
    total += size;
    if (total > MAX_TOTAL_BYTES) throw new Error('zip: the archive inflates past 1 GiB');
    const directory = name.endsWith('/');
    const data = directory
      ? Buffer.alloc(0)
      : entryData(zip, zip.readUInt32LE(at + 42), method, compressed, size);
    entries.push({ name: checkedName(name), directory, data });
    at += 46 + nameLength + zip.readUInt16LE(at + 30) + zip.readUInt16LE(at + 32);
  }
  return entries;
}
