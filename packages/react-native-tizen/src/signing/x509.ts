import { createSign, randomBytes, type KeyObject } from 'node:crypto';
import * as der from './der.ts';

/** The parts of an X.509 name a Tizen certificate uses. */
export interface DistinguishedName {
  readonly commonName: string;
  readonly organization?: string;
  readonly organizationalUnit?: string;
  readonly locality?: string;
  readonly state?: string;
  readonly country?: string;
  readonly email?: string;
}

export interface CertificateRequest {
  readonly subject: DistinguishedName;
  readonly publicKey: KeyObject;
  /** Who signs it; the subject itself, with `selfKey`, when absent. */
  readonly issuer?: { readonly name: DistinguishedName; readonly key: KeyObject };
  readonly selfKey?: KeyObject;
  readonly days: number;
  readonly from?: Date;
  /** A CA certificate may issue others; a leaf signs packages. */
  readonly authority?: boolean;
}

const NAME_OID: Readonly<Record<keyof DistinguishedName, string>> = {
  country: '2.5.4.6',
  state: '2.5.4.8',
  locality: '2.5.4.7',
  organization: '2.5.4.10',
  organizationalUnit: '2.5.4.11',
  commonName: '2.5.4.3',
  email: '1.2.840.113549.1.9.1',
};
const NAME_ORDER = [
  'country',
  'state',
  'locality',
  'organization',
  'organizationalUnit',
  'commonName',
  'email',
] as const;
const SHA256_WITH_RSA = '1.2.840.113549.1.1.11';
const BASIC_CONSTRAINTS = '2.5.29.19';
const KEY_USAGE = '2.5.29.15';
const DAY_MS = 86_400_000;
const DIGITAL_SIGNATURE = Buffer.from([0x80]);
const CERTIFICATE_SIGN = Buffer.from([0x06]);

function nameValue(field: keyof DistinguishedName, value: string): Buffer {
  if (field === 'country') return der.printable(value.toUpperCase());
  if (field === 'email') return der.encode(der.TAG.ia5String, Buffer.from(value, 'ascii'));
  return der.utf8(value);
}

/** An X.509 name, its attributes in the order OpenSSL prints them. */
export function encodeName(name: DistinguishedName): Buffer {
  const parts = NAME_ORDER.flatMap((field) => {
    const value = name[field];
    if (value === undefined || value === '') return [];
    return [der.set(der.sequence(der.oid(NAME_OID[field]), nameValue(field, value)))];
  });
  return der.sequence(...parts);
}

function extension(id: string, value: Buffer): Buffer {
  return der.sequence(der.oid(id), der.boolean(true), der.octets(value));
}

/** An X.509 v3 certificate in DER, signed with RSA and SHA-256. */
export function issueCertificate(request: CertificateRequest): Buffer {
  const signer = request.issuer?.key ?? request.selfKey;
  if (signer === undefined)
    throw new Error('x509: a self-signed certificate needs its own private key');
  const from = request.from ?? new Date();
  const until = new Date(from.getTime() + request.days * DAY_MS);
  const algorithm = der.sequence(der.oid(SHA256_WITH_RSA), der.nul());
  const subject = encodeName(request.subject);
  const issuer = request.issuer === undefined ? subject : encodeName(request.issuer.name);
  const constraints = request.authority === true ? der.sequence(der.boolean(true)) : der.sequence();
  const usage =
    request.authority === true
      ? der.bitString(CERTIFICATE_SIGN, 1)
      : der.bitString(DIGITAL_SIGNATURE, 7);
  const serial = randomBytes(8);
  serial[0] = (serial[0] ?? 0) & 0x7f;
  const tbs = der.sequence(
    der.explicit(0, der.integer(2)),
    der.integer(serial),
    algorithm,
    issuer,
    der.sequence(der.utcTime(from), der.utcTime(until)),
    subject,
    request.publicKey.export({ format: 'der', type: 'spki' }),
    der.explicit(
      3,
      der.sequence(extension(BASIC_CONSTRAINTS, constraints), extension(KEY_USAGE, usage)),
    ),
  );
  const signature = createSign('sha256').update(tbs).sign(signer);
  return der.sequence(tbs, algorithm, der.bitString(signature));
}
