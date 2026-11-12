import { createCipheriv, createHmac, pbkdf2Sync } from 'node:crypto';
import { describe, expect, it } from 'vitest';
import * as der from './der.ts';
import { OID, Pkcs12PasswordError, readPkcs12 } from './pkcs12.ts';
import { bmpPassword, KDF_PURPOSE, pkcs12Kdf } from './pkcs12-kdf.ts';
import { rc2CbcEncrypt } from './rc2-encrypt.fixtures.ts';
import { testSigner } from './signing.fixtures.ts';

const signer = testSigner('PKCS#12 schemes');
const pkcs8 = signer.key.export({ format: 'der', type: 'pkcs8' });
const SALT = Buffer.alloc(8, 7);
const IV = Buffer.alloc(16, 9);

const dataContent = (bags: Buffer) =>
  der.sequence(der.oid(OID.data), der.explicit(0, der.octets(bags)));
const authSafe = (...contents: Buffer[]) => der.octets(der.sequence(...contents));
const pkcs12 = (...contents: Buffer[]) =>
  der.sequence(
    der.integer(3),
    der.sequence(der.oid(OID.data), der.explicit(0, authSafe(...contents))),
  );
const keyBag = der.sequence(der.oid(OID.keyBag), der.explicit(0, pkcs8));
const shroudedKey = (algorithm: Buffer, encrypted: Buffer) =>
  dataContent(
    der.sequence(
      der.sequence(
        der.oid(OID.shroudedKeyBag),
        der.explicit(0, der.sequence(algorithm, der.octets(encrypted))),
      ),
    ),
  );
const pbe = (oid: string, iterations = 2048) =>
  der.sequence(der.oid(oid), der.sequence(der.octets(SALT), der.integer(iterations)));
const pbes2 = (kdfOid: string, cipherOid: string, prf?: Buffer) =>
  der.sequence(
    der.oid(OID.pbes2),
    der.sequence(
      der.sequence(
        der.oid(kdfOid),
        der.sequence(der.octets(SALT), der.integer(2048), ...(prf === undefined ? [] : [prf])),
      ),
      der.sequence(der.oid(cipherOid), der.octets(IV)),
    ),
  );
const pkcs12Key = (bits: number) =>
  pkcs12Kdf('sha1', bmpPassword('secret'), SALT, 2048, KDF_PURPOSE.key, bits / 8);
const pkcs12Iv = pkcs12Kdf('sha1', bmpPassword('secret'), SALT, 2048, KDF_PURPOSE.iv, 8);

describe('readPkcs12 schemes', () => {
  it('derives a PBES2 key with HMAC-SHA1 when the file names no PRF', () => {
    const key = pbkdf2Sync('secret', SALT, 2048, 32, 'sha1');
    const cipher = createCipheriv('aes-256-cbc', key, IV);
    const encrypted = Buffer.concat([cipher.update(pkcs8), cipher.final()]);

    const contents = readPkcs12(
      pkcs12(shroudedKey(pbes2(OID.pbkdf2, OID.aes256Cbc), encrypted)),
      'secret',
    );

    expect(contents.key.export({ format: 'der', type: 'pkcs8' }).equals(pkcs8)).toBe(true);
  });

  it('reads a key encrypted with 128-bit RC2, and takes a bad padding as a wrong password', () => {
    const encrypted = rc2CbcEncrypt(pkcs8, pkcs12Key(128), pkcs12Iv, 128);
    const file = pkcs12(shroudedKey(pbe(OID.pbeSha1Rc2_128), encrypted));

    expect(readPkcs12(file, 'secret').key.asymmetricKeyType).toBe('rsa');
    expect(() => readPkcs12(file, 'guess')).toThrow(Pkcs12PasswordError);
  });

  it('checks a MAC without an iteration count as one iteration', () => {
    const authenticated = der.sequence(dataContent(der.sequence(keyBag)));
    const macKey = pkcs12Kdf('sha1', bmpPassword('secret'), SALT, 1, KDF_PURPOSE.mac, 20);
    const mac = createHmac('sha1', macKey).update(authenticated).digest();
    const macData = der.sequence(
      der.sequence(der.sequence(der.oid(OID.sha1), der.nul()), der.octets(mac)),
      der.octets(SALT),
    );
    const file = der.sequence(
      der.integer(3),
      der.sequence(der.oid(OID.data), der.explicit(0, der.octets(authenticated))),
      macData,
    );

    expect(readPkcs12(file, 'secret').certificates).toEqual([]);
    expect(() => readPkcs12(file, 'guess')).toThrow(Pkcs12PasswordError);
  });

  it('reads encrypted content split into pieces, and skips bags it does not use', () => {
    const encrypted = rc2CbcEncrypt(
      der.sequence(
        der.sequence(
          der.oid(OID.certBag),
          der.explicit(0, der.sequence(der.oid(OID.data), der.explicit(0, der.octets(pkcs8)))),
        ),
        der.sequence(der.oid(OID.friendlyName), der.explicit(0, der.nul())),
      ),
      pkcs12Key(40),
      pkcs12Iv,
      40,
    );
    const pieces = der.encode(
      0xa0,
      Buffer.concat([der.octets(encrypted.subarray(0, 8)), der.octets(encrypted.subarray(8))]),
    );
    const encryptedData = der.sequence(
      der.oid(OID.encryptedData),
      der.explicit(
        0,
        der.sequence(
          der.integer(0),
          der.sequence(der.oid(OID.data), pbe(OID.pbeSha1Rc2_40), pieces),
        ),
      ),
    );

    const contents = readPkcs12(pkcs12(encryptedData, dataContent(der.sequence(keyBag))), 'secret');

    expect(contents.certificates).toEqual([]);
  });
});

describe('readPkcs12 refusals', () => {
  const encrypted = Buffer.alloc(16);

  it.each([
    ['an iteration count of zero', pbe(OID.pbeSha1TripleDes, 0), 'unreasonable iteration count'],
    ['an unknown PBE scheme', pbe(OID.data), 'unsupported encryption'],
    ['PBES2 with another KDF', pbes2(OID.data, OID.aes256Cbc), 'PBES2 without PBKDF2'],
    ['PBES2 with another cipher', pbes2(OID.pbkdf2, OID.data), 'unsupported PBES2 cipher'],
    [
      'PBES2 with an unknown PRF',
      pbes2(OID.pbkdf2, OID.aes256Cbc, der.sequence(der.oid(OID.data), der.nul())),
      'unsupported digest',
    ],
  ])('refuses %s', (_, algorithm, message) => {
    expect(() => readPkcs12(pkcs12(shroudedKey(algorithm, encrypted)), 'secret')).toThrow(message);
  });

  it('refuses a signed authenticated safe, content of another type and unframed ciphertext', () => {
    const signed = der.sequence(
      der.integer(3),
      der.sequence(der.oid(OID.encryptedData), der.explicit(0, der.octets(Buffer.alloc(0)))),
    );
    const otherContent = der.sequence(der.oid(OID.pbes2), der.explicit(0, der.nul()));
    const unframed = der.sequence(
      der.oid(OID.encryptedData),
      der.explicit(
        0,
        der.sequence(
          der.integer(0),
          der.sequence(der.oid(OID.data), pbe(OID.pbeSha1Rc2_40), der.octets(encrypted)),
        ),
      ),
    );

    expect(() => readPkcs12(signed, 'secret')).toThrow('signed authenticated safe');
    expect(() => readPkcs12(pkcs12(otherContent), 'secret')).toThrow('unsupported content');
    expect(() => readPkcs12(pkcs12(unframed), 'secret')).toThrow('expected encrypted content');
  });
});
