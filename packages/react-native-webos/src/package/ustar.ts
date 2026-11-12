import { FixableError } from '../errors.ts';

/** One entry of a tar archive: a file with its bytes, or a directory. */
export interface TarEntry {
  /** Relative, `/` separated, without a trailing `/`. */
  readonly path: string;
  readonly mode: number;
  /** A directory when absent. */
  readonly data?: Uint8Array;
}

/** An entry as {@link readTar} finds it, with the header fields a writer chose. */
export interface TarRecord {
  readonly path: string;
  readonly type: 'file' | 'directory' | 'symlink';
  readonly mode: number;
  readonly uid: number;
  readonly gid: number;
  readonly mtime: number;
  readonly data: Uint8Array;
  readonly linkTarget: string;
}

const BLOCK = 512;
const NAME_SIZE = 100;
const PREFIX_SIZE = 155;
const TYPE_FILE = '0';
const TYPE_SYMLINK = '2';
const TYPE_DIRECTORY = '5';
const TYPE_GNU_LONG_NAME = 'L';
const TYPE_PAX = 'x';
const SKIPPED_TYPES = new Set(['g', 'K']);
const RENAME_HINT = 'Rename the file, or move it nearer the root of the app.';

function splitPath(path: string): { name: string; prefix: string } {
  if (path.length <= NAME_SIZE) return { name: path, prefix: '' };
  for (let slash = path.indexOf('/'); slash !== -1; slash = path.indexOf('/', slash + 1)) {
    const name = path.slice(slash + 1);
    if (name !== '' && name.length <= NAME_SIZE && slash <= PREFIX_SIZE) {
      return { name, prefix: path.slice(0, slash) };
    }
  }
  throw new FixableError(`${path} is too long for a webOS package`, RENAME_HINT);
}

function octal(value: number, size: number): string {
  return `${value.toString(8).padStart(size - 1, '0')}\0`;
}

function header(path: string, entry: TarEntry, mtime: number): Buffer {
  // LG's own packager refuses a name outside ASCII, because opkg does not read
  // every POSIX tar (docs/reference/webos-ipk-format.md, section 4).
  if (!/^[\x20-\x7e]+$/.test(path)) {
    throw new FixableError(
      `${entry.path} has a character outside ASCII, which a webOS package cannot hold`,
      'Rename the file with English letters, digits and punctuation.',
    );
  }
  const { name, prefix } = splitPath(path);
  const block = Buffer.alloc(BLOCK);
  block.write(name, 0, 'latin1');
  block.write(octal(entry.mode, 8), 100, 'latin1');
  block.write(octal(0, 8), 108, 'latin1');
  block.write(octal(0, 8), 116, 'latin1');
  block.write(octal(entry.data?.byteLength ?? 0, 12), 124, 'latin1');
  block.write(octal(mtime, 12), 136, 'latin1');
  block.write(' '.repeat(8), 148, 'latin1');
  block.write(entry.data === undefined ? TYPE_DIRECTORY : TYPE_FILE, 156, 'latin1');
  block.write('ustar\x0000', 257, 'latin1');
  block.write(prefix, 345, 'latin1');
  const checksum = block.reduce((sum, byte) => sum + byte, 0);
  block.write(`${checksum.toString(8).padStart(6, '0')}\0 `, 148, 'latin1');
  return block;
}

function compareCodeUnits(left: string, right: string): number {
  if (left === right) return 0;
  return left < right ? -1 : 1;
}

/**
 * Writes `entries` as a plain ustar archive: sorted by path, every entry dated
 * `mtime` (seconds) and owned by uid and gid 0, a long path split between
 * `prefix` and `name`. Refuses a path ustar cannot hold rather than write a
 * PAX header.
 */
export function writeTar(entries: readonly TarEntry[], mtime: number): Buffer {
  const sorted = [...entries].sort((left, right) => compareCodeUnits(left.path, right.path));
  const blocks: Uint8Array[] = [];
  for (const entry of sorted) {
    const path = entry.data === undefined ? `${entry.path}/` : entry.path;
    blocks.push(header(path, entry, mtime));
    if (entry.data === undefined) continue;
    blocks.push(entry.data, Buffer.alloc((BLOCK - (entry.data.byteLength % BLOCK)) % BLOCK));
  }
  blocks.push(Buffer.alloc(2 * BLOCK));
  return Buffer.concat(blocks);
}

function text(block: Buffer, start: number, length: number): string {
  const field = block.subarray(start, start + length);
  const end = field.indexOf(0);
  return field.toString('latin1', 0, end === -1 ? length : end);
}

function number(block: Buffer, start: number, length: number): number {
  const digits = text(block, start, length).trim();
  return digits === '' ? 0 : Number.parseInt(digits, 8);
}

function paxPath(record: Buffer): string | null {
  const match = /(?:^|\n)\d+ path=([^\n]*)\n/.exec(record.toString('utf8'));
  return match?.[1] ?? null;
}

function recordType(flag: string): TarRecord['type'] {
  if (flag === TYPE_DIRECTORY) return 'directory';
  return flag === TYPE_SYMLINK ? 'symlink' : 'file';
}

/** The entries of a tar archive, ustar or GNU, with the long names either format writes. */
export function readTar(archive: Uint8Array): TarRecord[] {
  const bytes = Buffer.from(archive.buffer, archive.byteOffset, archive.byteLength);
  const records: TarRecord[] = [];
  let longPath: string | null = null;
  for (let offset = 0; offset + BLOCK <= bytes.byteLength;) {
    const block = bytes.subarray(offset, offset + BLOCK);
    if (block.every((byte) => byte === 0)) break;
    const size = number(block, 124, 12);
    const data = bytes.subarray(offset + BLOCK, offset + BLOCK + size);
    if (data.byteLength < size) throw new Error('A tar entry runs past the archive');
    offset += BLOCK + Math.ceil(size / BLOCK) * BLOCK;
    const flag = text(block, 156, 1) || TYPE_FILE;
    if (flag === TYPE_GNU_LONG_NAME) longPath = text(data, 0, data.byteLength);
    else if (flag === TYPE_PAX) longPath = paxPath(data) ?? longPath;
    if (flag === TYPE_GNU_LONG_NAME || flag === TYPE_PAX || SKIPPED_TYPES.has(flag)) continue;
    const prefix = text(block, 257, 6) === 'ustar' ? text(block, 345, PREFIX_SIZE) : '';
    const name = text(block, 0, NAME_SIZE);
    records.push({
      path: longPath ?? (prefix === '' ? name : `${prefix}/${name}`),
      type: recordType(flag),
      mode: number(block, 100, 8),
      uid: number(block, 108, 8),
      gid: number(block, 116, 8),
      mtime: number(block, 136, 12),
      data,
      linkTarget: text(block, 157, NAME_SIZE),
    });
    longPath = null;
  }
  return records;
}
