import { describe, expect, it } from 'vitest';
import { rc2CbcDecrypt, rc2DecryptBlock } from './rc2.ts';

const VECTORS = [
  { key: '0000000000000000', bits: 63, plain: '0000000000000000', cipher: 'ebb773f993278eff' },
  { key: 'ffffffffffffffff', bits: 64, plain: 'ffffffffffffffff', cipher: '278b27e42e2f0d49' },
  { key: '3000000000000000', bits: 64, plain: '1000000000000001', cipher: '30649edf9be7d2c2' },
  { key: '88', bits: 64, plain: '0000000000000000', cipher: '61a8a244adacccf0' },
  { key: '88bca90e90875a', bits: 64, plain: '0000000000000000', cipher: '6ccf4308974c267f' },
  {
    key: '88bca90e90875a7f0f79c384627bafb2',
    bits: 64,
    plain: '0000000000000000',
    cipher: '1a807d272bbe5db1',
  },
  {
    key: '88bca90e90875a7f0f79c384627bafb2',
    bits: 128,
    plain: '0000000000000000',
    cipher: '2269552ab0f85ca6',
  },
];

describe('rc2DecryptBlock', () => {
  it.each(VECTORS)(
    'decrypts the RFC 2268 vector for key $key at $bits bits',
    ({ key, bits, plain, cipher }) => {
      const decrypted = rc2DecryptBlock(Buffer.from(cipher, 'hex'), Buffer.from(key, 'hex'), bits);

      expect(decrypted.toString('hex')).toBe(plain);
    },
  );
});

describe('rc2CbcDecrypt', () => {
  it('refuses a ciphertext that is not whole blocks', () => {
    expect(() => rc2CbcDecrypt(Buffer.alloc(7), Buffer.alloc(5), Buffer.alloc(8), 40)).toThrow(
      'whole number',
    );
  });

  it('reads bad padding as a wrong password', () => {
    expect(() => rc2CbcDecrypt(Buffer.alloc(8), Buffer.alloc(5), Buffer.alloc(8), 40)).toThrow(
      'password',
    );
  });
});
