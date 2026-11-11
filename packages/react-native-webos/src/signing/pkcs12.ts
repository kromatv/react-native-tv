import {
  createDecipheriv,
  createHmac,
  createPrivateKey,
  pbkdf2Sync,
  timingSafeEqual,
  X509Certificate,
  type KeyObject,
} from 'node:crypto';
import { encode, TAG } from './der.ts';
import {
  childrenOf,
  explicitOf,
  octetsOf,
  oidOf,
  readDer,
  sequenceOf,
  smallIntegerOf,
  type DerNode,
} from './der-reader.ts';
import { bmpPassword, KDF_PURPOSE, pkcs12Kdf } from './pkcs12-kdf.ts';
import { rc2CbcDecrypt } from './rc2.ts';

/** The private key and the certificates a PKCS#12 file holds. */
export interface Pkcs12Contents {
  readonly key: KeyObject;
  readonly certificates: readonly X509Certificate[];
}

/** The password does not open the file: its MAC or its padding does not check out. */
export class Pkcs12PasswordError extends Error {
  constructor() {
    super('PKCS#12: wrong password');
    this.name = 'Pkcs12PasswordError';
  }
}

export const OID = {
  data: '1.2.840.113549.1.7.1',
  encryptedData: '1.2.840.113549.1.7.6',
  keyBag: '1.2.840.113549.1.12.10.1.1',
  shroudedKeyBag: '1.2.840.113549.1.12.10.1.2',
  certBag: '1.2.840.113549.1.12.10.1.3',
  x509Certificate: '1.2.840.113549.1.9.22.1',
  friendlyName: '1.2.840.113549.1.9.20',
  localKeyId: '1.2.840.113549.1.9.21',
  sha1: '1.3.14.3.2.26',
  sha256: '2.16.840.1.101.3.4.2.1',
  sha512: '2.16.840.1.101.3.4.2.3',
  pbeSha1TripleDes: '1.2.840.113549.1.12.1.3',
  pbeSha1Rc2_128: '1.2.840.113549.1.12.1.5',
  pbeSha1Rc2_40: '1.2.840.113549.1.12.1.6',
  pbes2: '1.2.840.113549.1.5.13',
  pbkdf2: '1.2.840.113549.1.5.12',
  hmacSha1: '1.2.840.113549.2.7',
  hmacSha256: '1.2.840.113549.2.9',
  hmacSha512: '1.2.840.113549.2.11',
  aes128Cbc: '2.16.840.1.101.3.4.1.2',
  aes256Cbc: '2.16.840.1.101.3.4.1.42',
} as const;

const MAX_ITERATIONS = 10_000_000;
const DIGESTS: Record<string, 'sha1' | 'sha256' | 'sha512'> = {
  [OID.sha1]: 'sha1',
  [OID.sha256]: 'sha256',
  [OID.sha512]: 'sha512',
  [OID.hmacSha1]: 'sha1',
  [OID.hmacSha256]: 'sha256',
  [OID.hmacSha512]: 'sha512',
};
const AES: Record<string, { readonly name: string; readonly bytes: number }> = {
  [OID.aes128Cbc]: { name: 'aes-128-cbc', bytes: 16 },
  [OID.aes256Cbc]: { name: 'aes-256-cbc', bytes: 32 },
};

function iterationsOf(node: DerNode | undefined): number {
  const iterations = smallIntegerOf(node);
  if (iterations < 1 || iterations > MAX_ITERATIONS)
    throw new Error('PKCS#12: an unreasonable iteration count');
  return iterations;
}

function digestOf(oid: string): 'sha1' | 'sha256' | 'sha512' {
  const digest = DIGESTS[oid];
  if (digest === undefined) throw new Error(`PKCS#12: unsupported digest ${oid}`);
  return digest;
}

function decipher(name: string, key: Buffer, iv: Buffer, data: Buffer): Buffer {
  try {
    const cipher = createDecipheriv(name, key, iv);
    return Buffer.concat([cipher.update(data), cipher.final()]);
  } catch {
    throw new Pkcs12PasswordError();
  }
}

function decryptPbes2(parameters: DerNode | undefined, password: string, data: Buffer): Buffer {
  const [kdf, scheme] = sequenceOf(parameters, 'PBES2 parameters');
  const [kdfOid, kdfParameters] = sequenceOf(kdf, 'a key derivation');
  if (oidOf(kdfOid) !== OID.pbkdf2) throw new Error('PKCS#12: PBES2 without PBKDF2');
  const [salt, iterations, ...rest] = sequenceOf(kdfParameters, 'PBKDF2 parameters');
  const prf = rest.find((node) => node.tag === TAG.sequence);
  const digest = prf === undefined ? 'sha1' : digestOf(oidOf(sequenceOf(prf, 'a PRF')[0]));
  const [schemeOid, iv] = sequenceOf(scheme, 'an encryption scheme');
  const aes = AES[oidOf(schemeOid)];
  if (aes === undefined) throw new Error('PKCS#12: unsupported PBES2 cipher');
  const key = pbkdf2Sync(
    Buffer.from(password, 'utf8'),
    octetsOf(salt),
    iterationsOf(iterations),
    aes.bytes,
    digest,
  );
  return decipher(aes.name, key, octetsOf(iv), data);
}

/** Decrypts `data` with the password-based scheme `algorithm` names. */
function decrypt(algorithm: DerNode | undefined, password: string, data: Buffer): Buffer {
  const [oidNode, parameters] = sequenceOf(algorithm, 'an encryption algorithm');
  const oid = oidOf(oidNode);
  if (oid === OID.pbes2) return decryptPbes2(parameters, password, data);
  const [saltNode, iterationsNode] = sequenceOf(parameters, 'PBE parameters');
  const salt = octetsOf(saltNode);
  const iterations = iterationsOf(iterationsNode);
  const secret = bmpPassword(password);
  const derive = (purpose: number, length: number) =>
    pkcs12Kdf('sha1', secret, salt, iterations, purpose, length);
  const iv = derive(KDF_PURPOSE.iv, 8);
  if (oid === OID.pbeSha1TripleDes)
    return decipher('des-ede3-cbc', derive(KDF_PURPOSE.key, 24), iv, data);
  const rc2Bits = oid === OID.pbeSha1Rc2_40 ? 40 : 128;
  if (oid !== OID.pbeSha1Rc2_40 && oid !== OID.pbeSha1Rc2_128) {
    throw new Error(`PKCS#12: unsupported encryption ${oid}`);
  }
  try {
    return rc2CbcDecrypt(data, derive(KDF_PURPOSE.key, rc2Bits / 8), iv, rc2Bits);
  } catch {
    throw new Pkcs12PasswordError();
  }
}

function verifyMac(macData: DerNode, password: string, authenticated: Buffer): void {
  const [digestInfo, saltNode, iterationsNode] = sequenceOf(macData, 'MAC data');
  const [algorithm, expected] = sequenceOf(digestInfo, 'a MAC digest');
  const digest = digestOf(oidOf(sequenceOf(algorithm, 'a MAC algorithm')[0]));
  const expectedMac = octetsOf(expected);
  const iterations = iterationsNode === undefined ? 1 : iterationsOf(iterationsNode);
  const key = pkcs12Kdf(
    digest,
    bmpPassword(password),
    octetsOf(saltNode),
    iterations,
    KDF_PURPOSE.mac,
    expectedMac.length,
  );
  const actual = createHmac(digest, key).update(authenticated).digest();
  if (actual.length !== expectedMac.length || !timingSafeEqual(actual, expectedMac)) {
    throw new Pkcs12PasswordError();
  }
}

/** The bytes of a `[0] IMPLICIT OCTET STRING`, primitive or split into pieces. */
function implicitOctets(node: DerNode | undefined): Buffer {
  if (node?.tag === 0x80) return node.contents;
  if (node?.tag === 0xa0) return Buffer.concat(childrenOf(node).map((piece) => octetsOf(piece)));
  throw new Error('PKCS#12: expected encrypted content');
}

type Opened = X509Certificate | KeyObject | null;

/**
 * Parses what `password` decrypted. In a file without a MAC a wrong password
 * shows only here: its CBC padding checks out about once in 256 tries, and
 * the bytes that come out do not parse.
 */
function parseDecrypted<T>(macChecked: boolean, parse: () => T): T {
  if (macChecked) return parse();
  try {
    return parse();
  } catch (error) {
    if (error instanceof Pkcs12PasswordError) throw error;
    throw new Pkcs12PasswordError();
  }
}

/** The certificate or private key a safe bag holds; null for a bag of another kind. */
function openBag(bag: DerNode, password: string, macChecked: boolean): Opened {
  const [bagId, bagValue] = sequenceOf(bag, 'a safe bag');
  const value = explicitOf(bagValue, 0);
  const id = oidOf(bagId);
  if (id === OID.certBag) {
    const [certType, certValue] = sequenceOf(value, 'a certificate bag');
    if (oidOf(certType) !== OID.x509Certificate) return null;
    return new X509Certificate(octetsOf(explicitOf(certValue, 0)));
  }
  if (id === OID.shroudedKeyBag) {
    const [algorithm, encrypted] = sequenceOf(value, 'an encrypted private key');
    const pkcs8 = decrypt(algorithm, password, octetsOf(encrypted));
    return parseDecrypted(macChecked, () =>
      createPrivateKey({ key: pkcs8, format: 'der', type: 'pkcs8' }),
    );
  }
  if (id !== OID.keyBag) return null;
  return createPrivateKey({ key: encode(value.tag, value.contents), format: 'der', type: 'pkcs8' });
}

function openContents(contentInfo: DerNode, password: string, macChecked: boolean): Opened[] {
  const [type, content] = sequenceOf(contentInfo, 'a content info');
  const oid = oidOf(type);
  const openBags = (bags: Buffer) =>
    sequenceOf(readDer(bags), 'safe contents').map((bag) => openBag(bag, password, macChecked));
  if (oid === OID.data) return openBags(octetsOf(explicitOf(content, 0)));
  if (oid !== OID.encryptedData) throw new Error(`PKCS#12: unsupported content ${oid}`);
  const [, encryptedContentInfo] = sequenceOf(explicitOf(content, 0), 'encrypted data');
  const [, algorithm, encrypted] = sequenceOf(encryptedContentInfo, 'encrypted content');
  const plain = decrypt(algorithm, password, implicitOctets(encrypted));
  return parseDecrypted(macChecked, () => openBags(plain));
}

/**
 * Opens a PKCS#12 file: checks its MAC with `password` before it decrypts
 * anything, then decrypts its key and certificates. Takes the PBE schemes of
 * RFC 7292 (3DES, RC2) and PBES2 with AES. A wrong password is a `Pkcs12PasswordError`.
 */
export function readPkcs12(der: Buffer, password: string): Pkcs12Contents {
  const [version, authSafe, macData] = sequenceOf(readDer(der), 'a PKCS#12 file');
  if (smallIntegerOf(version) !== 3) throw new Error('PKCS#12: not version 3');
  const [authType, authContent] = sequenceOf(authSafe, 'the authenticated safe');
  if (oidOf(authType) !== OID.data) {
    throw new Error('PKCS#12: a signed authenticated safe is not supported');
  }
  const authenticated = octetsOf(explicitOf(authContent, 0));
  if (macData !== undefined) verifyMac(macData, password, authenticated);
  const macChecked = macData !== undefined;
  let key: KeyObject | null = null;
  const certificates: X509Certificate[] = [];
  for (const contentInfo of sequenceOf(readDer(authenticated), 'content infos')) {
    for (const found of openContents(contentInfo, password, macChecked)) {
      if (found instanceof X509Certificate) certificates.push(found);
      else if (found !== null) key = found;
    }
  }
  if (key === null) throw new Error('PKCS#12: the file holds no private key');
  return { key, certificates };
}
