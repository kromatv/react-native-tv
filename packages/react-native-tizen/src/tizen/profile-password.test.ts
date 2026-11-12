import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { MissingProgramError, run } from '../exec.ts';
import { profilePassword } from './profile-password.ts';

vi.mock('../exec.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../exec.ts')>()),
  run: vi.fn(),
}));

const dirs: string[] = [];

afterEach(() => {
  vi.mocked(run).mockReset();
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('profilePassword', () => {
  it('takes a password written in the profile as it is, and none for an empty field', async () => {
    expect(await profilePassword('secret')).toBe('secret');
    expect(await profilePassword('')).toBeNull();
  });

  it('reads a .pwd file that exists', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-pwd-'));
    dirs.push(dir);
    const file = join(dir, 'author.pwd');
    writeFileSync(file, 'from-file\n');

    expect(await profilePassword(file, 'linux')).toBe('from-file');
  });

  it('finds no password for a missing .pwd file off macOS, nor in a keychain without it', async () => {
    const missing = join(tmpdir(), 'rn-tizen-missing', 'nobody.pwd');
    vi.mocked(run).mockRejectedValue(new MissingProgramError('/usr/bin/security'));

    expect(await profilePassword(missing, 'linux')).toBeNull();
    expect(await profilePassword(missing, 'darwin')).toBeNull();
    expect(run).toHaveBeenCalledOnce();
  });

  it('reads the macOS keychain under the .pwd path, and passes on what it does not expect', async () => {
    const missing = join(tmpdir(), 'rn-tizen-missing', 'author.pwd');
    vi.mocked(run).mockResolvedValueOnce('from-keychain\n').mockResolvedValueOnce('\n');

    expect(await profilePassword(missing, 'darwin')).toBe('from-keychain');
    expect(vi.mocked(run).mock.calls[0]?.[0]).toContain(missing);
    expect(await profilePassword(missing, 'darwin')).toBeNull();
    vi.mocked(run).mockRejectedValueOnce(new Error('disk on fire'));
    await expect(profilePassword(missing, 'darwin')).rejects.toThrow('disk on fire');
  });
});
