import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { writePkcs12 } from '../signing/pkcs12-write.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { checkCertificates } from './tizen-checks.ts';

let dir = '';
let previous: string | undefined;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-doctor-'));
  previous = process.env.TIZEN_SDK_DATA_DIR;
  process.env.TIZEN_SDK_DATA_DIR = dir;
});

afterEach(() => {
  if (previous === undefined) delete process.env.TIZEN_SDK_DATA_DIR;
  else process.env.TIZEN_SDK_DATA_DIR = previous;
  rmSync(dir, { recursive: true, force: true });
});

describe('checkCertificates', () => {
  it('fails with how to get certificates when there is no profile', async () => {
    const result = await checkCertificates();

    expect(result).toMatchObject({ status: 'fail', title: 'Certificates' });
  });

  it('passes with the active profile and when its distributor expires', async () => {
    const author = testSigner('Author');
    const distributor = testSigner('Distributor');
    writeFileSync(join(dir, 'a.p12'), writePkcs12(author.key, author.chain, 'pw', 'a'));
    writeFileSync(join(dir, 'd.p12'), writePkcs12(distributor.key, distributor.chain, 'pw', 'd'));
    mkdirSync(join(dir, 'profile'));
    writeFileSync(
      join(dir, 'profile', 'profiles.xml'),
      `<profiles active="TV" version="3.1"><profile name="TV">
<profileitem ca="" distributor="0" key="${join(dir, 'a.p12')}" password="pw" rootca=""/>
<profileitem ca="" distributor="1" key="${join(dir, 'd.p12')}" password="pw" rootca=""/>
</profile></profiles>`,
    );

    const result = await checkCertificates();

    expect(result.status).toBe('ok');
    expect(result.detail).toMatch(/^TV \(active\), distributor until \d{4}-\d{2}-\d{2}$/);
  });
});
