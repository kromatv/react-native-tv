/** The universal DER tags certificates and PKCS#12 files use. */
export const TAG = {
  boolean: 0x01,
  integer: 0x02,
  bitString: 0x03,
  octetString: 0x04,
  null: 0x05,
  oid: 0x06,
  utf8String: 0x0c,
  printableString: 0x13,
  ia5String: 0x16,
  utcTime: 0x17,
  bmpString: 0x1e,
  sequence: 0x30,
  set: 0x31,
} as const;

const CONTEXT_CONSTRUCTED = 0xa0;

function unsigned(value: number): Buffer {
  const bytes: number[] = [];
  let rest = value;
  do {
    bytes.unshift(rest & 0xff);
    rest = Math.floor(rest / 256);
  } while (rest > 0);
  return Buffer.from(bytes);
}

function encodeLength(size: number): Buffer {
  if (size < 0x80) return Buffer.from([size]);
  const bytes = unsigned(size);
  return Buffer.concat([Buffer.from([0x80 | bytes.length]), bytes]);
}

/** One DER value: its tag, its length and its contents. */
export function encode(tag: number, contents: Buffer): Buffer {
  return Buffer.concat([Buffer.from([tag]), encodeLength(contents.length), contents]);
}

export const sequence = (...parts: Buffer[]) => encode(TAG.sequence, Buffer.concat(parts));
export const set = (...parts: Buffer[]) => encode(TAG.set, Buffer.concat(parts));
export const nul = () => encode(TAG.null, Buffer.alloc(0));
export const octets = (contents: Buffer) => encode(TAG.octetString, contents);
export const utf8 = (text: string) => encode(TAG.utf8String, Buffer.from(text, 'utf8'));
export const printable = (text: string) => encode(TAG.printableString, Buffer.from(text, 'ascii'));
export const boolean = (value: boolean) => encode(TAG.boolean, Buffer.from([value ? 0xff : 0]));

/** Explicit `[n]` tagging, as X.509 versions, extensions and PKCS#12 contents use. */
export const explicit = (index: number, contents: Buffer) =>
  encode(CONTEXT_CONSTRUCTED | index, contents);

/** A minimal two's-complement INTEGER: no redundant leading zeros, one added when the top bit is set. */
export function integer(value: Buffer | number): Buffer {
  const raw = typeof value === 'number' ? unsigned(value) : value;
  let start = 0;
  while (start < raw.length - 1 && raw[start] === 0) start++;
  const bytes = raw.length === 0 ? Buffer.from([0]) : raw.subarray(start);
  const negative = (bytes.readUInt8(0) & 0x80) !== 0;
  return encode(TAG.integer, negative ? Buffer.concat([Buffer.from([0]), bytes]) : bytes);
}

/** A BIT STRING, whose first byte counts the unused bits of the last one. */
export const bitString = (contents: Buffer, unusedBits = 0) =>
  encode(TAG.bitString, Buffer.concat([Buffer.from([unusedBits]), contents]));

function base128(value: number): number[] {
  const bytes = [value & 0x7f];
  let rest = Math.floor(value / 128);
  while (rest > 0) {
    bytes.unshift((rest & 0x7f) | 0x80);
    rest = Math.floor(rest / 128);
  }
  return bytes;
}

export function oid(dotted: string): Buffer {
  const arcs = dotted.split('.').map(Number);
  const head = arcs.slice(0, 2).reduce((total, arc) => total * 40 + arc, 0);
  return encode(TAG.oid, Buffer.from([head, ...arcs.slice(2).flatMap(base128)]));
}

/** `YYMMDDhhmmssZ`, how X.509 writes a date before 2050. */
export function utcTime(date: Date): Buffer {
  const pad = (value: number) => String(value).padStart(2, '0');
  const stamp = [
    date.getUTCFullYear() % 100,
    date.getUTCMonth() + 1,
    date.getUTCDate(),
    date.getUTCHours(),
    date.getUTCMinutes(),
    date.getUTCSeconds(),
  ]
    .map(pad)
    .join('');
  return encode(TAG.utcTime, Buffer.from(`${stamp}Z`, 'ascii'));
}
