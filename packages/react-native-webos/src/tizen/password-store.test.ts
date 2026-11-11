import { ChildProcess, spawn } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync, statSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { PassThrough } from 'node:stream';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { storePassword } from './password-store.ts';
import { profilePassword } from './profile-password.ts';

vi.mock('node:child_process', async (importOriginal) => ({
  ...(await importOriginal<typeof import('node:child_process')>()),
  spawn: vi.fn(),
}));
vi.mock('./profile-password.ts', async (importOriginal) => {
  const actual = await importOriginal<typeof import('./profile-password.ts')>();
  return { profilePassword: vi.fn(actual.profilePassword) };
});

const PWD = '/certificates/author.pwd';

function fakeSecurity(exitCode: number, stderr = ''): { input: () => string } {
  const child = new ChildProcess();
  const stdin = new PassThrough();
  const errors = new PassThrough();
  child.stdin = stdin;
  child.stderr = errors;
  let input = '';
  stdin.on('data', (chunk: Buffer) => {
    input += chunk.toString('utf8');
  });
  stdin.on('finish', () => {
    errors.end(stderr);
    errors.on('end', () => child.emit('close', exitCode));
    errors.resume();
  });
  vi.mocked(spawn).mockReturnValueOnce(child);
  return { input: () => input };
}

afterEach(() => {
  vi.mocked(spawn).mockReset();
});

describe('storePassword', () => {
  it('writes the .pwd file off macOS, readable by its owner only', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-store-'));
    const file = join(dir, 'author.pwd');
    try {
      await storePassword(file, 'p w"\\q', 'linux');

      expect(readFileSync(file, 'utf8')).toBe('p w"\\q');
      expect(statSync(file).mode & 0o777).toBe(0o600);
    } finally {
      rmSync(dir, { recursive: true, force: true });
    }
  });

  it('adds the password to the macOS keychain over stdin, quoted', async () => {
    const security = fakeSecurity(0);
    vi.mocked(profilePassword).mockResolvedValueOnce('p w"\\q');

    await storePassword(PWD, 'p w"\\q', 'darwin');

    expect(spawn).toHaveBeenCalledWith('/usr/bin/security', ['-i'], expect.anything());
    expect(security.input()).toBe(
      `add-generic-password -U -s certificate-manager -a "${PWD}" -w "p w\\"\\\\q"\n`,
    );
  });

  it('says what security printed, or its exit code, when it fails', async () => {
    fakeSecurity(1, 'denied\n');
    await expect(storePassword(PWD, 'secret', 'darwin')).rejects.toThrow(
      'security could not store the password: denied',
    );

    fakeSecurity(2);
    await expect(storePassword(PWD, 'secret', 'darwin')).rejects.toThrow('exit 2');
  });

  it('refuses a password that does not read back', async () => {
    fakeSecurity(0);
    vi.mocked(profilePassword).mockResolvedValueOnce(null);

    await expect(storePassword(PWD, 'secret', 'darwin')).rejects.toThrow('does not read back');
  });
});
