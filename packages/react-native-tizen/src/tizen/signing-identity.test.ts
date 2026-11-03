import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { FixableError } from '../errors.ts';
import { writePkcs12 } from '../signing/pkcs12-write.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { resolveSigning } from './signing-identity.ts';

const dirs: string[] = [];
const author = testSigner('Author');
const distributor = testSigner('Distributor');

function profiles(active: string, password = 'secret'): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-profiles-'));
  dirs.push(dir);
  writeFileSync(join(dir, 'author.p12'), writePkcs12(author.key, author.chain, 'secret', 'author'));
  writeFileSync(
    join(dir, 'distributor.p12'),
    writePkcs12(distributor.key, distributor.chain, 'secret', 'distributor'),
  );
  const path = join(dir, 'profiles.xml');
  writeFileSync(
    path,
    [
      `<profiles active="${active}" version="3.1">`,
      '<profile name="TV">',
      `<profileitem ca="" distributor="0" key="${join(dir, 'author.p12')}" password="${password}" rootca=""/>`,
      `<profileitem ca="" distributor="1" key="${join(dir, 'distributor.p12')}" password="${password}" rootca=""/>`,
      '<profileitem ca="" distributor="2" key="" password="" rootca=""/>',
      '</profile>',
      '</profiles>',
    ].join('\n'),
  );
  return path;
}

afterEach(() => {
  vi.useRealTimers();
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('resolveSigning', () => {
  it('opens the active profile: one author and one distributor', async () => {
    const resolved = await resolveSigning({}, profiles('TV'));

    expect(resolved.name).toBe('TV');
    expect(resolved.identity.author.chain[0]?.subject).toContain('Author Test Signer');
    expect(resolved.identity.distributors).toHaveLength(1);
    expect(resolved.distributor.subject).toContain('Distributor Test Signer');
  });

  it('names the profiles there are when the one asked for is missing', async () => {
    await expect(resolveSigning({ profile: 'Store' }, profiles('TV'))).rejects.toThrow(
      'No signing profile named Store',
    );
  });

  it('says which password failed', async () => {
    const signing = resolveSigning({}, profiles('TV', 'wrong'));

    await expect(signing).rejects.toBeInstanceOf(FixableError);
    await expect(signing).rejects.toThrow('The author password does not open');
  });

  it('asks for a profile when none is active', async () => {
    await expect(resolveSigning({}, profiles(''))).rejects.toThrow('No active signing profile');
  });
});

describe('resolveSigning with files given on the command line', () => {
  const saved = {
    author: process.env.RN_TIZEN_AUTHOR_PASSWORD,
    distributor: process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD,
  };

  afterEach(() => {
    process.env.RN_TIZEN_AUTHOR_PASSWORD = saved.author;
    process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD = saved.distributor;
    if (saved.author === undefined) delete process.env.RN_TIZEN_AUTHOR_PASSWORD;
    if (saved.distributor === undefined) delete process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD;
  });

  function files(): { author: string; distributor: string } {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-flags-'));
    dirs.push(dir);
    writeFileSync(join(dir, 'a.p12'), writePkcs12(author.key, author.chain, 'secret', 'a'));
    writeFileSync(
      join(dir, 'd.p12'),
      writePkcs12(distributor.key, distributor.chain, 'secret', 'd'),
    );
    return { author: join(dir, 'a.p12'), distributor: join(dir, 'd.p12') };
  }

  it('opens both halves with the passwords in the environment, without a profile', async () => {
    process.env.RN_TIZEN_AUTHOR_PASSWORD = 'secret';
    process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD = 'secret';

    const resolved = await resolveSigning(files(), '/nonexistent/profiles.xml');

    expect(resolved.name).toBe('the given certificates');
    expect(resolved.identity.distributors).toHaveLength(1);
  });

  it('asks for the password it does not have', async () => {
    delete process.env.RN_TIZEN_AUTHOR_PASSWORD;

    await expect(resolveSigning(files(), '/nonexistent/profiles.xml')).rejects.toThrow(
      'No password for the author certificate',
    );
  });

  it('says which file it could not read', async () => {
    process.env.RN_TIZEN_AUTHOR_PASSWORD = 'secret';
    process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD = 'secret';

    await expect(
      resolveSigning({ author: '/nonexistent/a.p12', distributor: files().distributor }, '/x'),
    ).rejects.toThrow('Could not read the author certificate /nonexistent/a.p12');
  });

  it('refuses a certificate that has expired', async () => {
    const path = profiles('TV');
    vi.useFakeTimers({ toFake: ['Date'] });
    vi.setSystemTime(new Date('2200-01-01T00:00:00Z'));

    await expect(resolveSigning({}, path)).rejects.toThrow(
      /The author certificate in .* expired on/,
    );
  });

  it('says the profile has no author certificate when its author entry is empty', async () => {
    const path = profiles('TV');
    const xml = readFileSync(path, 'utf8').replace(/key="[^"]*author\.p12"/, 'key=""');
    writeFileSync(path, xml);

    await expect(resolveSigning({}, path)).rejects.toThrow('The profile has no author certificate');
  });
});
