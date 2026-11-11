import { closeSync, openSync, readSync } from 'node:fs';

const ELF_MAGIC = 0x7f454c46;
const CLASS_32 = 1;
const CLASS_64 = 2;
const LITTLE_ENDIAN = 1;
const PT_LOAD = 1;
const HEADER_BYTES = 64;
const MAX_PROGRAM_HEADERS = 128;

interface Layout {
  readonly phoff: (header: Buffer) => number;
  readonly entrySizeAt: number;
  readonly countAt: number;
  readonly vaddr: (entry: Buffer) => bigint;
}

const LAYOUTS: Readonly<Record<number, Layout>> = {
  [CLASS_32]: {
    phoff: (header) => header.readUInt32LE(28),
    entrySizeAt: 42,
    countAt: 44,
    vaddr: (entry) => BigInt(entry.readUInt32LE(8)),
  },
  [CLASS_64]: {
    phoff: (header) => Number(header.readBigUInt64LE(32)),
    entrySizeAt: 54,
    countAt: 56,
    vaddr: (entry) => entry.readBigUInt64LE(16),
  },
};

function read(fd: number, position: number, length: number): Buffer | null {
  const buffer = Buffer.alloc(length);
  return readSync(fd, buffer, 0, length, position) === length ? buffer : null;
}

function lowestLoadAddress(fd: number): bigint | null {
  const header = read(fd, 0, HEADER_BYTES);
  if (header === null || header.readUInt32BE(0) !== ELF_MAGIC || header[5] !== LITTLE_ENDIAN) {
    return null;
  }
  const layout = LAYOUTS[header[4] ?? 0];
  if (layout === undefined) return null;
  const entrySize = header.readUInt16LE(layout.entrySizeAt);
  const count = Math.min(header.readUInt16LE(layout.countAt), MAX_PROGRAM_HEADERS);
  let lowest: bigint | null = null;
  for (let index = 0; index < count; index += 1) {
    const entry = read(fd, layout.phoff(header) + index * entrySize, entrySize);
    if (entry === null || entry.length < 24) return null;
    if (entry.readUInt32LE(0) !== PT_LOAD) continue;
    const address = layout.vaddr(entry);
    if (lowest === null || address < lowest) lowest = address;
  }
  return lowest;
}

/**
 * The address the first loaded segment of an ELF file is linked at: 0 for a
 * shared library or a position-independent executable, 0x10000 for the TV's
 * executable, which is linked at a fixed address. A crash report's offsets
 * count from where that segment is in memory, and a symbolizer wants the
 * addresses the file was linked with (docs/reference/webos-native-toolchain.md,
 * section 9). Null for a file that is no little-endian ELF.
 */
export function elfLoadBase(file: string): bigint | null {
  let fd: number;
  try {
    fd = openSync(file, 'r');
  } catch {
    return null;
  }
  try {
    return lowestLoadAddress(fd);
  } catch {
    return null;
  } finally {
    closeSync(fd);
  }
}
