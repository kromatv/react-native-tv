import { TAG } from './der.ts';

/** One DER value read out of a buffer. */
export interface DerNode {
  readonly tag: number;
  readonly contents: Buffer;
}

/** Reads the value at `offset`, and where the next one starts. Only definite lengths, as DER has. */
function readAt(buffer: Buffer, offset: number): { node: DerNode; end: number } {
  const tag = buffer[offset];
  const first = buffer[offset + 1];
  if (tag === undefined || first === undefined) throw new Error('DER: the data ends mid-value');
  let length = first;
  let header = 2;
  if (first & 0x80) {
    const count = first & 0x7f;
    if (count === 0 || count > 4) throw new Error('DER: an unsupported length');
    if (offset + 2 + count > buffer.length) throw new Error('DER: the data ends mid-value');
    length = buffer.readUIntBE(offset + 2, count);
    header += count;
  }
  const end = offset + header + length;
  if (end > buffer.length) throw new Error('DER: a length past the end of the data');
  return { node: { tag, contents: buffer.subarray(offset + header, end) }, end };
}

/** The single DER value `buffer` holds. */
export function readDer(buffer: Buffer): DerNode {
  const { node, end } = readAt(buffer, 0);
  if (end !== buffer.length) throw new Error('DER: data after the value');
  return node;
}

/** The values inside a constructed one (a SEQUENCE, a SET, an explicit tag). */
export function childrenOf(node: DerNode): DerNode[] {
  const children: DerNode[] = [];
  for (let offset = 0; offset < node.contents.length;) {
    const { node: child, end } = readAt(node.contents, offset);
    children.push(child);
    offset = end;
  }
  return children;
}

export function expectTag(node: DerNode | undefined, tag: number, what: string): DerNode {
  if (node?.tag !== tag) throw new Error(`DER: expected ${what}`);
  return node;
}

/** The children of a SEQUENCE, checked to be one. */
export function sequenceOf(node: DerNode | undefined, what: string): DerNode[] {
  return childrenOf(expectTag(node, TAG.sequence, what));
}

export function oidOf(node: DerNode | undefined): string {
  const { contents } = expectTag(node, TAG.oid, 'an object identifier');
  if (contents.length === 0) throw new Error('DER: an empty object identifier');
  const first = contents.readUInt8(0);
  const parts = [Math.floor(first / 40), first % 40];
  let value = 0;
  for (const byte of contents.subarray(1)) {
    value = value * 128 + (byte & 0x7f);
    if ((byte & 0x80) === 0) {
      parts.push(value);
      value = 0;
    }
  }
  return parts.join('.');
}

/** A non-negative INTEGER small enough for a number, such as an iteration count. */
export function smallIntegerOf(node: DerNode | undefined): number {
  const { contents } = expectTag(node, TAG.integer, 'an integer');
  if (contents.length === 0 || contents.length > 6 || (contents.readUInt8(0) & 0x80) !== 0)
    throw new Error('DER: an integer out of range');
  return contents.reduce((total, byte) => total * 256 + byte, 0);
}

export function octetsOf(node: DerNode | undefined): Buffer {
  return expectTag(node, TAG.octetString, 'an octet string').contents;
}

/** The value inside an explicit `[index]` tag. */
export function explicitOf(node: DerNode | undefined, index: number): DerNode {
  const [inner] = childrenOf(expectTag(node, 0xa0 | index, `[${index}]`));
  if (inner === undefined) throw new Error(`DER: an empty [${index}]`);
  return inner;
}
