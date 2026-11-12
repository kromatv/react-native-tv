import { generateKeyPairSync, X509Certificate, type KeyObject } from 'node:crypto';
import { issueCertificate } from './x509.ts';

/** A test key and a certificate for it, issued by a test CA that is itself in the chain. */
export interface TestSigner {
  readonly key: KeyObject;
  readonly chain: readonly X509Certificate[];
}

export function testSigner(label: string): TestSigner {
  const ca = generateKeyPairSync('rsa', { modulusLength: 2048 });
  const caName = { commonName: `${label} Test CA` };
  const caCertificate = new X509Certificate(
    issueCertificate({
      subject: caName,
      publicKey: ca.publicKey,
      selfKey: ca.privateKey,
      days: 30,
      authority: true,
    }),
  );
  const leaf = generateKeyPairSync('rsa', { modulusLength: 2048 });
  const certificate = new X509Certificate(
    issueCertificate({
      subject: { commonName: `${label} Test Signer`, country: 'CH' },
      publicKey: leaf.publicKey,
      issuer: { name: caName, key: ca.privateKey },
      days: 30,
    }),
  );
  return { key: leaf.privateKey, chain: [certificate, caCertificate] };
}
