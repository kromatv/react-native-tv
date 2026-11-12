import { createVerify, X509Certificate } from 'node:crypto';
import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { signFiles, packageEntries, signPackage } from './sign-package.ts';
import { readZip } from './zip-read.ts';
import { writeZip } from './zip-write.ts';
import type { SigningKey } from './signature-xml.ts';
import { FIXTURE_FILES } from './tizen-studio-signing.fixtures.ts';

const TIZEN_AUTHOR_XML = readFileSync(
  new URL('fixtures/author-signature.xml', import.meta.url),
  'utf8',
);
const TIZEN_DISTRIBUTOR_XML = readFileSync(
  new URL('fixtures/signature1.xml', import.meta.url),
  'utf8',
);

function between(text: string, open: string, close: string): string[] {
  return text
    .split(open)
    .slice(1)
    .map((part) => part.slice(0, part.indexOf(close)));
}

/**
 * Signs as Tizen Studio did, without its private key: the signature value is
 * Tizen Studio's own, handed back only when it verifies against the bytes
 * this code asks to be signed.
 */
function replay(document: string): SigningKey {
  const chain = between(document, '<X509Certificate>\n', '\n</X509Certificate>').map(
    (base64) => new X509Certificate(Buffer.from(base64.replaceAll('\n', ''), 'base64')),
  );
  const [value = ''] = between(document, '<SignatureValue>\n', '\n</SignatureValue>');
  const signature = Buffer.from(value.replaceAll('\n', ''), 'base64');
  return {
    chain,
    sign: (data) => {
      const verifies = createVerify('RSA-SHA512')
        .update(data)
        .verify(chain[0]?.publicKey ?? '', signature);
      if (!verifies) throw new Error('the bytes to sign differ from what Tizen Studio signed');
      return signature;
    },
  };
}

function fixtureFiles(): Map<string, Buffer> {
  return new Map(
    Object.entries(FIXTURE_FILES).map(([path, text]) => [path, Buffer.from(text, 'utf8')]),
  );
}

describe('signFiles', () => {
  it('writes the signatures `tizen package -t tpk` wrote, byte for byte', () => {
    const files = fixtureFiles();

    signFiles(files, {
      author: replay(TIZEN_AUTHOR_XML),
      distributors: [replay(TIZEN_DISTRIBUTOR_XML)],
    });

    expect(files.get('author-signature.xml')?.toString('utf8')).toBe(TIZEN_AUTHOR_XML);
    expect(files.get('signature1.xml')?.toString('utf8')).toBe(TIZEN_DISTRIBUTOR_XML);
  });

  it('replaces the signatures a package already carries', () => {
    const files = fixtureFiles();
    files.set('author-signature.xml', Buffer.from('stale'));
    files.set('signature2.xml', Buffer.from('stale'));

    signFiles(files, {
      author: replay(TIZEN_AUTHOR_XML),
      distributors: [replay(TIZEN_DISTRIBUTOR_XML)],
    });

    expect(files.get('author-signature.xml')?.toString('utf8')).toBe(TIZEN_AUTHOR_XML);
    expect(files.has('signature2.xml')).toBe(false);
  });
});

describe('packageEntries', () => {
  it('adds every parent directory and sorts the entries by name', () => {
    const files = new Map([
      ['res/a/b.txt', Buffer.from('b')],
      ['bin/App.dll', Buffer.from('a')],
    ]);

    expect(
      packageEntries(files).map((entry) => (entry.directory ? `${entry.name}/` : entry.name)),
    ).toEqual(['bin/', 'bin/App.dll', 'res/', 'res/a/', 'res/a/b.txt']);
  });
});

describe('signPackage', () => {
  it('rewrites a package file signed, as tizen package does in place', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-sign-'));
    const file = join(dir, 'app.tpk');
    const entries = Object.entries(FIXTURE_FILES).map(([name, text]) => ({
      name,
      directory: false,
      data: Buffer.from(text),
    }));
    writeFileSync(file, writeZip(entries));
    try {
      await signPackage(file, {
        author: replay(TIZEN_AUTHOR_XML),
        distributors: [replay(TIZEN_DISTRIBUTOR_XML)],
      });

      const signed = readZip(readFileSync(file));
      expect(signed.find((entry) => entry.name === 'signature1.xml')?.data.toString('utf8')).toBe(
        TIZEN_DISTRIBUTOR_XML,
      );
      expect(signed.some((entry) => entry.directory && entry.name === 'res')).toBe(true);
    } finally {
      rmSync(dir, { recursive: true, force: true });
    }
  });
});
