import {
  createCipheriv,
  createHash,
  createHmac,
  randomBytes,
  type KeyObject,
  type X509Certificate,
} from 'node:crypto';
import * as der from './der.ts';
import { OID } from './pkcs12.ts';
import { bmpPassword, KDF_PURPOSE, pkcs12Kdf } from './pkcs12-kdf.ts';

const ITERATIONS = 2048;
const SALT_BYTES = 16;

function bmpString(text: string): Buffer {
  return der.encode(der.TAG.bmpString, bmpPassword(text).subarray(0, text.length * 2));
}

function attributes(alias: string, keyId: Buffer): Buffer {
  return der.set(
    der.sequence(der.oid(OID.friendlyName), der.set(bmpString(alias))),
    der.sequence(der.oid(OID.localKeyId), der.set(der.octets(keyId))),
  );
}

function bag(id: string, value: Buffer, bagAttributes?: Buffer): Buffer {
  return der.sequence(
    der.oid(id),
    der.explicit(0, value),
    ...(bagAttributes === undefined ? [] : [bagAttributes]),
  );
}

function encrypt(
  data: Buffer,
  password: string,
): { readonly algorithm: Buffer; readonly encrypted: Buffer } {
  const salt = randomBytes(SALT_BYTES);
  const secret = bmpPassword(password);
  const key = pkcs12Kdf('sha1', secret, salt, ITERATIONS, KDF_PURPOSE.key, 24);
  const iv = pkcs12Kdf('sha1', secret, salt, ITERATIONS, KDF_PURPOSE.iv, 8);
  // oxlint-disable-next-line sonarjs/no-weak-cipher, sonarjs/encryption-secure-mode
  const cipher = createCipheriv('des-ede3-cbc', key, iv); // NOSONAR: the only cipher Tizen Studio opens
  const encrypted = Buffer.concat([cipher.update(data), cipher.final()]);
  const parameters = der.sequence(der.octets(salt), der.integer(ITERATIONS));
  return { algorithm: der.sequence(der.oid(OID.pbeSha1TripleDes), parameters), encrypted };
}

function dataContent(contents: Buffer): Buffer {
  return der.sequence(der.oid(OID.data), der.explicit(0, der.octets(contents)));
}

function encryptedContent(contents: Buffer, password: string): Buffer {
  const { algorithm, encrypted } = encrypt(contents, password);
  const info = der.sequence(der.oid(OID.data), algorithm, der.encode(0x80, encrypted));
  return der.sequence(
    der.oid(OID.encryptedData),
    der.explicit(0, der.sequence(der.integer(0), info)),
  );
}

/**
 * A PKCS#12 file holding one private key and its chain, leaf first. Tizen
 * Studio's certificate tools read only PBE-SHA1-3DES with a SHA-1 MAC, and
 * refuse AES and an HMAC-SHA256 MAC, so a file they can use is written that way.
 */
export function writePkcs12(
  key: KeyObject,
  chain: readonly X509Certificate[],
  password: string,
  alias: string,
): Buffer {
  const [leaf] = chain;
  if (leaf === undefined) throw new Error('PKCS#12: a key needs its certificate');
  const keyId = createHash('sha256').update(leaf.raw).digest().subarray(0, 20);
  const certificates = chain.map((certificate, index) =>
    bag(
      OID.certBag,
      der.sequence(der.oid(OID.x509Certificate), der.explicit(0, der.octets(certificate.raw))),
      index === 0 ? attributes(alias, keyId) : undefined,
    ),
  );
  const shrouded = encrypt(key.export({ format: 'der', type: 'pkcs8' }), password);
  const keyBag = bag(
    OID.shroudedKeyBag,
    der.sequence(shrouded.algorithm, der.octets(shrouded.encrypted)),
    attributes(alias, keyId),
  );
  const authenticated = der.sequence(
    encryptedContent(der.sequence(...certificates), password),
    dataContent(der.sequence(keyBag)),
  );
  const salt = randomBytes(SALT_BYTES);
  const macKey = pkcs12Kdf('sha1', bmpPassword(password), salt, ITERATIONS, KDF_PURPOSE.mac, 20);
  const mac = createHmac('sha1', macKey).update(authenticated).digest();
  const macData = der.sequence(
    der.sequence(der.sequence(der.oid(OID.sha1), der.nul()), der.octets(mac)),
    der.octets(salt),
    der.integer(ITERATIONS),
  );
  return der.sequence(der.integer(3), dataContent(authenticated), macData);
}
