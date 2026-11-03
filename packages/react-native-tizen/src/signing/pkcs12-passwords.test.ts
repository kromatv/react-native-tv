import { createCipheriv, createHmac, pbkdf2Sync, randomBytes } from 'node:crypto';
import { describe, expect, it } from 'vitest';
import * as der from './der.ts';
import { OID, Pkcs12PasswordError, readPkcs12 } from './pkcs12.ts';
import { bmpPassword, KDF_PURPOSE, pkcs12Kdf } from './pkcs12-kdf.ts';
import { writePkcs12 } from './pkcs12-write.ts';
import { rc2CbcEncrypt } from './rc2-encrypt.fixtures.ts';
import { testSigner } from './signing.fixtures.ts';

const PASSWORD = 'secret';
const WRONG = Array.from({ length: 512 }, (_, n) => `guess-${n}`);
const ITERATIONS = 1;
const signer = testSigner('PKCS#12 passwords');
const pkcs8 = signer.key.export({ format: 'der', type: 'pkcs8' });
const certificate = signer.chain[0]?.raw ?? Buffer.alloc(0);

const dataContent = (bags: Buffer) =>
  der.sequence(der.oid(OID.data), der.explicit(0, der.octets(bags)));
const pfx = (authenticated: Buffer, ...macData: Buffer[]) =>
  der.sequence(
    der.integer(3),
    der.sequence(der.oid(OID.data), der.explicit(0, der.octets(authenticated))),
    ...macData,
  );
const keyBag = der.sequence(der.oid(OID.keyBag), der.explicit(0, pkcs8));
const shroudedKeyBag = (algorithm: Buffer, encrypted: Buffer) =>
  der.sequence(
    der.oid(OID.shroudedKeyBag),
    der.explicit(0, der.sequence(algorithm, der.octets(encrypted))),
  );
const pbe = (oid: string, salt: Buffer) =>
  der.sequence(der.oid(oid), der.sequence(der.octets(salt), der.integer(ITERATIONS)));
const kdf = (salt: Buffer, purpose: number, length: number) =>
  pkcs12Kdf('sha1', bmpPassword(PASSWORD), salt, ITERATIONS, purpose, length);

/** What reading the file with each password ends in: 'password' for a Pkcs12PasswordError. */
function outcomes(file: Buffer, passwords: readonly string[]): Set<string> {
  return new Set(
    passwords.map((password) => {
      try {
        readPkcs12(file, password);
        return 'opened';
      } catch (error) {
        return error instanceof Pkcs12PasswordError ? 'password' : String(error);
      }
    }),
  );
}

function pbes2Key(): Buffer {
  const salt = randomBytes(16);
  const iv = randomBytes(16);
  const key = pbkdf2Sync(PASSWORD, salt, ITERATIONS, 32, 'sha256');
  const cipher = createCipheriv('aes-256-cbc', key, iv);
  const algorithm = der.sequence(
    der.oid(OID.pbes2),
    der.sequence(
      der.sequence(
        der.oid(OID.pbkdf2),
        der.sequence(
          der.octets(salt),
          der.integer(ITERATIONS),
          der.sequence(der.oid(OID.hmacSha256), der.nul()),
        ),
      ),
      der.sequence(der.oid(OID.aes256Cbc), der.octets(iv)),
    ),
  );
  return dataContent(
    der.sequence(shroudedKeyBag(algorithm, Buffer.concat([cipher.update(pkcs8), cipher.final()]))),
  );
}

function rc2Certificates(): Buffer {
  const salt = randomBytes(8);
  const bags = der.sequence(
    der.sequence(
      der.oid(OID.certBag),
      der.explicit(
        0,
        der.sequence(der.oid(OID.x509Certificate), der.explicit(0, der.octets(certificate))),
      ),
    ),
  );
  const encrypted = rc2CbcEncrypt(
    bags,
    kdf(salt, KDF_PURPOSE.key, 5),
    kdf(salt, KDF_PURPOSE.iv, 8),
    40,
  );
  return der.sequence(
    der.oid(OID.encryptedData),
    der.explicit(
      0,
      der.sequence(
        der.integer(0),
        der.sequence(der.oid(OID.data), pbe(OID.pbeSha1Rc2_40, salt), der.encode(0x80, encrypted)),
      ),
    ),
  );
}

describe('readPkcs12 with wrong passwords', () => {
  it('refuses every one by the MAC when the file has one', () => {
    const file = writePkcs12(signer.key, signer.chain, PASSWORD, 'author');

    expect(outcomes(file, WRONG.slice(0, 64))).toEqual(new Set(['password']));
  });

  it('refuses every one without a MAC, whatever its padding happens to let through', () => {
    const pbes2 = pfx(der.sequence(pbes2Key()));
    const rc2 = pfx(der.sequence(rc2Certificates(), dataContent(der.sequence(keyBag))));

    expect(outcomes(pbes2, WRONG)).toEqual(new Set(['password']));
    expect(outcomes(rc2, WRONG)).toEqual(new Set(['password']));
    expect(outcomes(pbes2, [PASSWORD])).toEqual(new Set(['opened']));
    expect(outcomes(rc2, [PASSWORD])).toEqual(new Set(['opened']));
  });

  it('reports what the right password decrypts to when it is not a key', () => {
    const salt = randomBytes(8);
    const cipher = createCipheriv(
      'des-ede3-cbc',
      kdf(salt, KDF_PURPOSE.key, 24),
      kdf(salt, KDF_PURPOSE.iv, 8),
    );
    const notAKey = Buffer.concat([cipher.update(Buffer.from('not a key')), cipher.final()]);
    const authenticated = der.sequence(
      dataContent(der.sequence(shroudedKeyBag(pbe(OID.pbeSha1TripleDes, salt), notAKey))),
    );
    const macSalt = randomBytes(8);
    const mac = createHmac('sha1', kdf(macSalt, KDF_PURPOSE.mac, 20))
      .update(authenticated)
      .digest();
    const file = pfx(
      authenticated,
      der.sequence(
        der.sequence(der.sequence(der.oid(OID.sha1), der.nul()), der.octets(mac)),
        der.octets(macSalt),
        der.integer(ITERATIONS),
      ),
    );

    expect(() => readPkcs12(file, 'guess')).toThrow(Pkcs12PasswordError);
    expect(() => readPkcs12(file, PASSWORD)).toThrow(Error);
    expect(() => readPkcs12(file, PASSWORD)).not.toThrow(Pkcs12PasswordError);
  });
});
