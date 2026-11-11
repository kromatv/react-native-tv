import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { createAuthorCertificate } from './author-certificate.ts';
import { readPkcs12 } from './pkcs12.ts';

describe('createAuthorCertificate', () => {
  it('writes a self-signed author certificate its password opens', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-author-'));
    try {
      const author = await createAuthorCertificate({
        directory: dir,
        subject: { commonName: 'ci' },
        days: 30,
      });
      const contents = readPkcs12(readFileSync(author.path), author.password);

      expect(author.certificate.subject).toBe('CN=ci');
      expect(author.certificate.issuer).toBe(author.certificate.subject);
      expect(contents.certificates[0]?.checkPrivateKey(contents.key)).toBe(true);
    } finally {
      rmSync(dir, { recursive: true, force: true });
    }
  });
});
