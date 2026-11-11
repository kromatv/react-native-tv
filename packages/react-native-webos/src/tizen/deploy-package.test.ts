import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { testSigner } from '../signing/signing.fixtures.ts';
import { signingKeyFrom } from '../signing/signing-key.ts';
import { writeZip } from '../signing/zip-write.ts';
import { openPackage, signedCopy } from './deploy-package.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function key(label: string) {
  const { key: privateKey, chain } = testSigner(label);
  return signingKeyFrom({ key: privateKey, certificates: [...chain] });
}

describe('signedCopy', () => {
  it('signs a widget in a copy, leaving the original as it was', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-deploy-'));
    dirs.push(dir);
    const file = join(dir, 'app.wgt');
    const config = '<widget><tizen:application id="AbCdE12345.App" package="AbCdE12345"/></widget>';
    writeFileSync(
      file,
      writeZip([
        { name: 'config.xml', directory: false, data: Buffer.from(config) },
        { name: 'index.html', directory: false, data: Buffer.from('<p>hi</p>') },
      ]),
    );

    const opened = await openPackage(file);
    const signed = await signedCopy(opened, {
      author: key('Author'),
      distributors: [key('Distributor')],
    });
    dirs.push(join(signed, '..'));

    expect(opened.info.installId).toBe('AbCdE12345.App');
    expect([...(await openPackage(signed)).files.keys()].sort()).toEqual([
      'author-signature.xml',
      'config.xml',
      'index.html',
      'signature1.xml',
    ]);
    expect([...(await openPackage(file)).files.keys()]).toEqual(['config.xml', 'index.html']);
  });
});
