import { blockWords, expandKey } from './rc2.ts';

const ROTATIONS = Buffer.from([1, 2, 3, 5]);

function encryptBlock(block: Buffer, keys: Buffer): Buffer {
  const { get: word, set, bytes } = blockWords(block);
  const key = (k: number) => keys.readUInt16LE(k * 2);
  let j = 0;
  const mixRound = () => {
    for (let i = 0; i < 4; i++) {
      set(i, word(i) + key(j) + (word(i - 1) & word(i - 2)) + (~word(i - 1) & word(i - 3)));
      j += 1;
      const shift = ROTATIONS.readUInt8(i);
      set(i, (word(i) << shift) | (word(i) >>> (16 - shift)));
    }
  };
  const mashRound = () => {
    for (let i = 0; i < 4; i++) set(i, word(i) + key(word(i - 1) & 63));
  };
  for (let round = 0; round < 5; round++) mixRound();
  mashRound();
  for (let round = 0; round < 6; round++) mixRound();
  mashRound();
  for (let round = 0; round < 5; round++) mixRound();
  return bytes;
}

/** RC2-CBC encryption with PKCS#7 padding, for building test files Node cannot. */
export function rc2CbcEncrypt(
  data: Buffer,
  key: Buffer,
  iv: Buffer,
  effectiveBits: number,
): Buffer {
  const keys = expandKey(key, effectiveBits);
  const padding = 8 - (data.length % 8);
  const plain = Buffer.concat([data, Buffer.alloc(padding, padding)]);
  const out = Buffer.alloc(plain.length);
  let previous = iv;
  for (let at = 0; at < plain.length; at += 8) {
    const mixed = Buffer.alloc(8);
    for (let i = 0; i < 8; i++) mixed[i] = plain.readUInt8(at + i) ^ previous.readUInt8(i);
    previous = encryptBlock(mixed, keys);
    previous.copy(out, at);
  }
  return out;
}
