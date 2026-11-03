const PITABLE = Buffer.from(
  'd978f9c419ddb5ed28e9fd794aa0d89dc67e37832b76538e624c6488448bfba2' +
    '179a59f587b34f1361456d8d09817d32bd8f40eb86b77b0bf09521225c6b4e82' +
    '54d66593ce60b21c7356c014a78cf1dc1275ca1f3bbee4d1423dd430a33cb626' +
    '6fbf0eda4669075727f21d9bbc944303f811c7f690ef3ee706c3d52fc8661ed7' +
    '08e8eade8052eef784aa72ac354d6a2a961ad2715a1549744b9fd05e0418a4ec' +
    'c2e0416e0f51cbcc2491af50a1f47039997c3a8523b8b47afc02365b25559731' +
    '2d5dfa98e38a92ae05df2910676cbac9d300e6cfe19ea82c6316013f58e289a9' +
    '0d38341bab33ffb0bb480c5fb9b1cd2ec5f3db47e5a59c770aa62068fe7fc1ad',
  'hex',
);

const BLOCK_BYTES = 8;
const ROTATIONS = Buffer.from([1, 2, 3, 5]);

const pi = (index: number) => PITABLE.readUInt8(index & 0xff);

/** RFC 2268's key expansion: the 64 16-bit little-endian words every round draws on. */
export function expandKey(key: Buffer, effectiveBits: number): Buffer {
  const expanded = Buffer.alloc(128);
  key.copy(expanded);
  for (let i = key.length; i < 128; i++) {
    expanded[i] = pi(expanded.readUInt8(i - 1) + expanded.readUInt8(i - key.length));
  }
  const effectiveBytes = (effectiveBits + 7) >> 3;
  const mask = 0xff >> (8 * effectiveBytes - effectiveBits);
  const first = 128 - effectiveBytes;
  expanded[first] = pi(expanded.readUInt8(first) & mask);
  for (let i = first - 1; i >= 0; i--) {
    expanded[i] = pi(expanded.readUInt8(i + 1) ^ expanded.readUInt8(i + effectiveBytes));
  }
  return expanded;
}

/** The four 16-bit words of a block, indexed modulo 4 as RFC 2268's R[i-1] needs. */
export function blockWords(block: Buffer) {
  const r = Buffer.from(block);
  return {
    get: (i: number) => r.readUInt16LE(((i + 4) % 4) * 2),
    set: (i: number, value: number) => r.writeUInt16LE(value & 0xffff, i * 2),
    bytes: r,
  };
}

function decryptBlock(block: Buffer, keys: Buffer): Buffer {
  const { get: word, set, bytes } = blockWords(block);
  const key = (k: number) => keys.readUInt16LE(k * 2);
  let j = 63;
  const mixRound = () => {
    for (let i = 3; i >= 0; i--) {
      const shift = ROTATIONS.readUInt8(i);
      const rotated = ((word(i) >>> shift) | (word(i) << (16 - shift))) & 0xffff;
      set(i, rotated - key(j) - (word(i - 1) & word(i - 2)) - (~word(i - 1) & word(i - 3)));
      j -= 1;
    }
  };
  const mashRound = () => {
    for (let i = 3; i >= 0; i--) set(i, word(i) - key(word(i - 1) & 63));
  };
  for (let round = 0; round < 5; round++) mixRound();
  mashRound();
  for (let round = 0; round < 6; round++) mixRound();
  mashRound();
  for (let round = 0; round < 5; round++) mixRound();
  return bytes;
}

/** RC2 (RFC 2268) on one 8-byte block, the raw cipher the CBC mode below chains. */
export function rc2DecryptBlock(block: Buffer, key: Buffer, effectiveBits: number): Buffer {
  return decryptBlock(block, expandKey(key, effectiveBits));
}

/**
 * RC2-CBC decryption with PKCS#7 padding removed. Node's OpenSSL 3 dropped
 * RC2, and the PKCS#12 files Samsung issues encrypt their certificates with it.
 */
export function rc2CbcDecrypt(
  data: Buffer,
  key: Buffer,
  iv: Buffer,
  effectiveBits: number,
): Buffer {
  if (data.length === 0 || data.length % BLOCK_BYTES !== 0 || iv.length !== BLOCK_BYTES) {
    throw new Error('RC2: the ciphertext is not a whole number of blocks');
  }
  const keys = expandKey(key, effectiveBits);
  const plain = Buffer.alloc(data.length);
  let previous = iv;
  for (let at = 0; at < data.length; at += BLOCK_BYTES) {
    const block = data.subarray(at, at + BLOCK_BYTES);
    const decrypted = decryptBlock(block, keys);
    for (let i = 0; i < BLOCK_BYTES; i++)
      plain[at + i] = decrypted.readUInt8(i) ^ previous.readUInt8(i);
    previous = block;
  }
  const padding = plain.readUInt8(plain.length - 1);
  if (
    padding < 1 ||
    padding > BLOCK_BYTES ||
    plain.subarray(-padding).some((byte) => byte !== padding)
  ) {
    throw new Error('RC2: bad padding, so the password is probably wrong');
  }
  return plain.subarray(0, plain.length - padding);
}
