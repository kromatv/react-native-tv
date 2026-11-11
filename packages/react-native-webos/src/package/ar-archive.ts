/** One member of an `ar` archive. */
export interface ArMember {
  readonly name: string;
  readonly data: Uint8Array;
}

/** A member as {@link readAr} finds it, with the header fields a writer chose. */
export interface ArEntry extends ArMember {
  readonly mtime: number;
  readonly uid: number;
  readonly gid: number;
  readonly mode: string;
}

const GLOBAL_HEADER = '!<arch>\n';
const HEADER_SIZE = 60;
const HEADER_END = '`\n';
const MEMBER_MODE = '100644';
const NAME_SIZE = 16;

function memberHeader(name: string, size: number, mtime: number): Buffer {
  // A webOS 4 TV refuses a package whose member names end in `/` as GNU ar
  // writes them, so names are bare (docs/reference/webos-ipk-format.md, section 1).
  if (!/^[\x21-\x7e]+$/.test(name) || name.includes('/') || name.length > NAME_SIZE) {
    throw new Error(`${name} is not a name an ar member can have`);
  }
  const fields = [
    name.padEnd(NAME_SIZE),
    String(mtime).padEnd(12),
    '0'.padEnd(6),
    '0'.padEnd(6),
    MEMBER_MODE.padEnd(8),
    String(size).padEnd(10),
    HEADER_END,
  ];
  return Buffer.from(fields.join(''), 'latin1');
}

/** Writes `members`, in order, as an `ar` archive dated `mtime` (seconds), the way `ares-package` does. */
export function writeAr(members: readonly ArMember[], mtime: number): Buffer {
  const parts: Uint8Array[] = [Buffer.from(GLOBAL_HEADER, 'latin1')];
  for (const member of members) {
    parts.push(memberHeader(member.name, member.data.byteLength, mtime), member.data);
    if (member.data.byteLength % 2 === 1) parts.push(Buffer.from('\n', 'latin1'));
  }
  return Buffer.concat(parts);
}

function headerField(header: Buffer, start: number, length: number): string {
  return header.toString('latin1', start, start + length).trimEnd();
}

function decimalField(header: Buffer, start: number, length: number, what: string): number {
  const text = headerField(header, start, length);
  if (!/^\d+$/.test(text)) throw new Error(`An ar member has no ${what}`);
  return Number(text);
}

/** The members of an `ar` archive, names as written: a GNU archive's keep their trailing `/`. */
export function readAr(archive: Uint8Array): ArEntry[] {
  const bytes = Buffer.from(archive.buffer, archive.byteOffset, archive.byteLength);
  if (bytes.toString('latin1', 0, GLOBAL_HEADER.length) !== GLOBAL_HEADER) {
    throw new Error('Not an ar archive');
  }
  const entries: ArEntry[] = [];
  let offset = GLOBAL_HEADER.length;
  while (offset < bytes.byteLength) {
    const header = bytes.subarray(offset, offset + HEADER_SIZE);
    if (header.byteLength < HEADER_SIZE || header.toString('latin1', 58) !== HEADER_END) {
      throw new Error(`The ar archive has no member header at byte ${offset}`);
    }
    const size = decimalField(header, 48, 10, 'size');
    const start = offset + HEADER_SIZE;
    if (start + size > bytes.byteLength) throw new Error('An ar member runs past the archive');
    entries.push({
      name: headerField(header, 0, NAME_SIZE),
      mtime: decimalField(header, 16, 12, 'time'),
      uid: decimalField(header, 28, 6, 'uid'),
      gid: decimalField(header, 34, 6, 'gid'),
      mode: headerField(header, 40, 8),
      data: bytes.subarray(start, start + size),
    });
    offset = start + size + (size % 2);
  }
  return entries;
}
