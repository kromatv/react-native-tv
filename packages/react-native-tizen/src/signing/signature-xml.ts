import { createHash, type X509Certificate } from 'node:crypto';

/** Who a signature speaks for: the author, or a distributor. */
export type SignatureRole = 'author' | 'distributor';

/** One signed file: its URI as the signature writes it, and its bytes. */
export interface SignedFile {
  readonly uri: string;
  readonly bytes: Buffer;
}

/** A certificate chain, leaf first, and the RSA-SHA512 signature of its leaf's private key. */
export interface SigningKey {
  readonly chain: readonly X509Certificate[];
  sign(data: Buffer): Buffer;
}

const DSIG = 'http://www.w3.org/2000/09/xmldsig#';
const EXCLUSIVE_C14N = 'http://www.w3.org/2001/10/xml-exc-c14n#';
const RSA_SHA512 = 'http://www.w3.org/2001/04/xmldsig-more#rsa-sha512';
const SHA512 = 'http://www.w3.org/2001/04/xmlenc#sha512';
const C14N11 = 'http://www.w3.org/2006/12/xml-c14n11';
const PROPERTIES = 'http://www.w3.org/2009/xmldsig-properties';
const DIGSIG = 'http://www.w3.org/ns/widgets-digsig';
const BASE64_LINE = /.{76}/g;

const wrap = (bytes: Buffer) => bytes.toString('base64').replace(BASE64_LINE, '$&\n').trimEnd();
const digest = (bytes: Buffer) => wrap(createHash('sha512').update(bytes).digest());

const property = (name: string, target: string, value: string) =>
  `<SignatureProperty Id="${name}" Target="#${target}">${value}</SignatureProperty>`;

function signatureProperties(id: string, role: SignatureRole): string {
  return [
    `<SignatureProperties xmlns:dsp="${PROPERTIES}">`,
    property('profile', id, `<dsp:Profile URI="${DIGSIG}#profile"></dsp:Profile>`),
    property('role', id, `<dsp:Role URI="${DIGSIG}#role-${role}"></dsp:Role>`),
    property('identifier', id, '<dsp:Identifier></dsp:Identifier>'),
    '</SignatureProperties>',
  ].join('');
}

const objectElement = (properties: string, namespace: string) =>
  `<Object${namespace} Id="prop">${properties}</Object>`;

const fileReference = (file: SignedFile) =>
  [
    `<Reference URI="${file.uri}">`,
    `<DigestMethod Algorithm="${SHA512}"></DigestMethod>`,
    `<DigestValue>${digest(file.bytes)}</DigestValue>`,
    '</Reference>',
  ].join('\n');

const propertiesReference = (value: string) =>
  [
    '<Reference URI="#prop">',
    '<Transforms>',
    `<Transform Algorithm="${C14N11}"></Transform>`,
    '</Transforms>',
    `<DigestMethod Algorithm="${SHA512}"></DigestMethod>`,
    `<DigestValue>${value}</DigestValue>`,
    '</Reference>',
  ].join('\n');

/**
 * One signature document as `tizen package` writes it, already in canonical
 * form: `SignedInfo` and the `prop` object are hashed as written, with the
 * namespace exclusive canonicalization gives them, instead of being re-serialized.
 */
export function signatureXml(
  files: readonly SignedFile[],
  role: SignatureRole,
  signer: SigningKey,
): string {
  const id = role === 'author' ? 'AuthorSignature' : 'DistributorSignature';
  const properties = signatureProperties(id, role);
  const propertiesDigest = digest(
    Buffer.from(objectElement(properties, ` xmlns="${DSIG}"`), 'utf8'),
  );
  const signedInfo = (open: string) =>
    [
      open,
      `<CanonicalizationMethod Algorithm="${EXCLUSIVE_C14N}"></CanonicalizationMethod>`,
      `<SignatureMethod Algorithm="${RSA_SHA512}"></SignatureMethod>`,
      ...files.map(fileReference),
      propertiesReference(propertiesDigest),
      '</SignedInfo>',
    ].join('\n');
  const value = signer.sign(Buffer.from(signedInfo(`<SignedInfo xmlns="${DSIG}">`), 'utf8'));
  return [
    `<Signature xmlns="${DSIG}" Id="${id}">`,
    signedInfo('<SignedInfo>'),
    `<SignatureValue>\n${wrap(value)}\n</SignatureValue>`,
    '<KeyInfo>',
    '<X509Data>',
    ...signer.chain.map(
      (certificate) => `<X509Certificate>\n${wrap(certificate.raw)}\n</X509Certificate>`,
    ),
    '</X509Data>',
    '</KeyInfo>',
    objectElement(properties, ''),
    '</Signature>',
  ].join('\n');
}
