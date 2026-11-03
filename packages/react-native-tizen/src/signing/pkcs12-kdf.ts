import { createHash } from 'node:crypto';

/** What a PKCS#12 key derivation is for: RFC 7292 appendix B.3's ID byte. */
export const KDF_PURPOSE = { key: 1, iv: 2, mac: 3 } as const;

const BLOCK_BYTES: Record<'sha1' | 'sha256' | 'sha512', number> = {
  sha1: 64,
  sha256: 64,
  sha512: 128,
};

/** A password as PKCS#12 hashes it: UTF-16 big-endian with a two-byte terminator. */
export function bmpPassword(password: string): Buffer {
  return Buffer.concat([Buffer.from(password, 'utf16le').swap16(), Buffer.alloc(2)]);
}

function repeatTo(source: Buffer, blockBytes: number): Buffer {
  if (source.length === 0) return source;
  const length = Math.ceil(source.length / blockBytes) * blockBytes;
  const out = Buffer.alloc(length);
  for (let i = 0; i < length; i++) out[i] = source.readUInt8(i % source.length);
  return out;
}

/** RFC 7292 appendix B.2: `length` pseudorandom bytes from a password, a salt and a purpose. */
export function pkcs12Kdf(
  hash: 'sha1' | 'sha256' | 'sha512',
  password: Buffer,
  salt: Buffer,
  iterations: number,
  purpose: number,
  length: number,
): Buffer {
  const v = BLOCK_BYTES[hash];
  const diversifier = Buffer.alloc(v, purpose);
  const input = Buffer.concat([repeatTo(salt, v), repeatTo(password, v)]);
  const output: Buffer[] = [];
  let produced = 0;
  while (produced < length) {
    let digest = createHash(hash).update(diversifier).update(input).digest();
    for (let i = 1; i < iterations; i++) digest = createHash(hash).update(digest).digest();
    output.push(digest);
    produced += digest.length;
    const b = repeatTo(digest, v).subarray(0, v);
    for (let block = 0; block < input.length; block += v) {
      let carry = 1;
      for (let i = v - 1; i >= 0; i--) {
        const sum = input.readUInt8(block + i) + b.readUInt8(i) + carry;
        input[block + i] = sum & 0xff;
        carry = sum >> 8;
      }
    }
  }
  return Buffer.concat(output).subarray(0, length);
}
