import { generateKeyPairSync, randomBytes, X509Certificate } from 'node:crypto';
import { writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { writePkcs12 } from './pkcs12-write.ts';
import { issueCertificate, type DistinguishedName } from './x509.ts';

/** A generated author certificate: its PKCS#12 file and the password that opens it. */
export interface AuthorCertificate {
  readonly path: string;
  readonly password: string;
  readonly certificate: X509Certificate;
}

export interface AuthorRequest {
  readonly directory: string;
  readonly subject: DistinguishedName;
  readonly days?: number;
}

const KEY_BITS = 2048;
const VALID_DAYS = 1825;

/**
 * A self-signed author certificate: an RSA key and an X.509 certificate in a
 * PKCS#12 file Tizen Studio can open, under a random password. Tizen's
 * validator and a retail Samsung TV refuse it: it suits packages that are
 * signed again before they are installed, such as a CI build.
 */
export async function createAuthorCertificate(request: AuthorRequest): Promise<AuthorCertificate> {
  const { publicKey, privateKey } = generateKeyPairSync('rsa', { modulusLength: KEY_BITS });
  const certificate = new X509Certificate(
    issueCertificate({
      subject: request.subject,
      publicKey,
      selfKey: privateKey,
      days: request.days ?? VALID_DAYS,
    }),
  );
  const password = randomBytes(24).toString('base64url');
  const path = join(request.directory, 'author.p12');
  await writeFile(path, writePkcs12(privateKey, [certificate], password, 'author'), {
    mode: 0o600,
  });
  return { path, password, certificate };
}
