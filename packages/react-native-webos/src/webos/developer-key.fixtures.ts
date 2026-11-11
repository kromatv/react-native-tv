import { createCipheriv, createHash, generateKeyPairSync, randomBytes } from 'node:crypto';
import { createServer, type Server } from 'node:http';

/** An RSA key as `ssh2` and the fake TV read it: unprotected PKCS#1 PEM. */
export function rsaKeyPem(): string {
  const { privateKey } = generateKeyPairSync('rsa', { modulusLength: 2048 });
  return privateKey.export({ type: 'pkcs1', format: 'pem' }).toString();
}

function pemBody(pem: string): Buffer {
  return Buffer.from(pem.replace(/-----[A-Z ]+-----/g, '').replace(/\s+/g, ''), 'base64');
}

/**
 * Protects a PKCS#1 key the way a TV's key server does (webos-device-protocol.md,
 * section 2): OpenSSL's traditional PEM encryption, AES-128-CBC under
 * MD5(passphrase || IV[0..8]), with the IV in `DEK-Info`.
 */
export function protectKey(pem: string, phrase: string): string {
  const iv = randomBytes(16);
  // oxlint-disable-next-line sonarjs/hashing
  const key = createHash('md5').update(phrase).update(iv.subarray(0, 8)).digest(); // NOSONAR: the format's own derivation
  // oxlint-disable-next-line sonarjs/encryption-secure-mode
  const cipher = createCipheriv('aes-128-cbc', key, iv); // NOSONAR: the format's own cipher
  const body = Buffer.concat([cipher.update(pemBody(pem)), cipher.final()]).toString('base64');
  return [
    '-----BEGIN RSA PRIVATE KEY-----',
    'Proc-Type: 4,ENCRYPTED',
    `DEK-Info: AES-128-CBC,${iv.toString('hex').toUpperCase()}`,
    '',
    ...(body.match(/.{1,64}/g) ?? []),
    '-----END RSA PRIVATE KEY-----',
    '',
  ].join('\n');
}

/** A local HTTP server that answers `/webos_rsa` as a TV's key server, and 404 elsewhere. */
export interface FakeKeyServer {
  readonly port: number;
  readonly requests: string[];
  close(): Promise<void>;
}

export async function fakeKeyServer(body: string, status = 200): Promise<FakeKeyServer> {
  const requests: string[] = [];
  const server: Server = createServer((request, response) => {
    requests.push(`${request.method} ${request.url}`);
    const found = request.url === '/webos_rsa';
    response.writeHead(found ? status : 404, { 'content-type': 'application/octet-stream' });
    response.end(found ? body : '');
  });
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
  const address = server.address();
  if (address === null || typeof address === 'string') throw new Error('the key server got no port');
  return {
    port: address.port,
    requests,
    close: () =>
      new Promise((resolve) => {
        server.closeAllConnections();
        server.close(() => resolve());
      }),
  };
}
