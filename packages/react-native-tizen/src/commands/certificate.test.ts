import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { writePkcs12 } from '../signing/pkcs12-write.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { certificateCommand } from './certificate.ts';

vi.mock('../tizen/password-store.ts', async (original) => {
  const real = await original<typeof import('../tizen/password-store.ts')>();
  return {
    storePassword: (file: string, password: string) => real.storePassword(file, password, 'linux'),
  };
});

const VARIABLES = [
  'HOME',
  'TIZEN_SDK_DATA_DIR',
  'XDG_STATE_HOME',
  'RN_TIZEN_AUTHOR_PASSWORD',
  'RN_TIZEN_DISTRIBUTOR_PASSWORD',
];
let dir = '';
let saved: Record<string, string | undefined> = {};
let printed = '';

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-certificate-'));
  saved = Object.fromEntries(VARIABLES.map((name) => [name, process.env[name]]));
  process.env.HOME = join(dir, 'home');
  process.env.TIZEN_SDK_DATA_DIR = join(dir, 'data');
  process.env.XDG_STATE_HOME = join(dir, 'state');
  printed = '';
  vi.spyOn(process.stdout, 'write').mockImplementation((chunk) => {
    printed += String(chunk);
    return true;
  });
});

afterEach(() => {
  vi.restoreAllMocks();
  for (const [name, value] of Object.entries(saved)) {
    if (value === undefined) delete process.env[name];
    else process.env[name] = value;
  }
  rmSync(dir, { recursive: true, force: true });
});

const run = (...args: string[]) => certificateCommand.run(args, { cwd: dir });

function samsungFiles(): { author: string; distributor: string } {
  const author = testSigner('Author');
  const distributor = testSigner('Distributor');
  writeFileSync(join(dir, 'author.p12'), writePkcs12(author.key, author.chain, 'a-pass', 'author'));
  writeFileSync(
    join(dir, 'distributor.p12'),
    writePkcs12(distributor.key, distributor.chain, 'd-pass', 'distributor'),
  );
  return { author: 'author.p12', distributor: 'distributor.p12' };
}

describe('rn-tizen certificate', () => {
  it('says how to import certificates when there is no profile', async () => {
    await run();

    expect(printed).toContain('rn-tizen certificate import');
  });

  it('imports Samsung certificates as the active profile, lists it and switches back to it', async () => {
    const files = samsungFiles();
    process.env.RN_TIZEN_AUTHOR_PASSWORD = 'a-pass';
    process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD = 'd-pass';

    await run('import', 'TV', '--author', files.author, '--distributor', files.distributor);
    delete process.env.RN_TIZEN_AUTHOR_PASSWORD;
    delete process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD;
    printed = '';
    await run('list');
    await run('use', 'TV');

    const profiles = readFileSync(join(dir, 'data', 'profile', 'profiles.xml'), 'utf8');
    expect(profiles).toContain('active="TV"');
    expect(existsSync(join(dir, 'home', 'SamsungCertificate', 'TV', 'distributor.p12'))).toBe(true);
    expect(printed).toContain('Distributor Test Signer, issued by Distributor Test CA');
    await expect(
      run('import', 'TV', '--author', files.author, '--distributor', files.distributor),
    ).rejects.toThrow('already exists');
  });

  it('lists a profile whose files are gone with what keeps it from opening', async () => {
    const files = samsungFiles();
    process.env.RN_TIZEN_AUTHOR_PASSWORD = 'a-pass';
    process.env.RN_TIZEN_DISTRIBUTOR_PASSWORD = 'd-pass';
    await run('import', 'TV', '--author', files.author, '--distributor', files.distributor);
    rmSync(join(dir, 'home', 'SamsungCertificate', 'TV', 'author.p12'));
    printed = '';

    await run('list');

    expect(printed).toContain('TV');
    expect(printed).toContain('author.p12');
    expect(printed).not.toContain('Distributor Test Signer');
  });

  it('makes an author certificate, and refuses a name already used', async () => {
    await run('create-author', 'ci');

    expect(
      existsSync(join(dir, 'state', 'react-native-tizen', 'certificates', 'ci', 'author.p12')),
    ).toBe(true);
    expect(printed).toContain('refuses a self-signed author certificate');
    await expect(run('create-author', 'ci')).rejects.toThrow('already exists');
  });

  it('refuses an unknown action, a bad name and an import without both files', async () => {
    await expect(run('lsit')).rejects.toThrow('Did you mean list?');
    await expect(run('use', '../x')).rejects.toThrow('needs one profile name');
    await expect(run('import', 'TV')).rejects.toThrow('needs --author and --distributor');
  });
});
