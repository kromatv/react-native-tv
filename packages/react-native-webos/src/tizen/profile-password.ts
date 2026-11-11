import { existsSync, readFileSync } from 'node:fs';
import { z } from 'zod';
import { CommandError, MissingProgramError, run } from '../exec.ts';

const passwordSchema = z.string().min(1).max(1024);
const KEYCHAIN_SERVICE = 'certificate-manager';

async function fromKeychain(account: string): Promise<string | null> {
  try {
    const output = await run(
      ['/usr/bin/security', 'find-generic-password', '-s', KEYCHAIN_SERVICE, '-a', account, '-w'],
      {
        quiet: true,
      },
    );
    const parsed = passwordSchema.safeParse(output.replace(/\r?\n$/, ''));
    return parsed.success ? parsed.data : null;
  } catch (error) {
    if (error instanceof CommandError || error instanceof MissingProgramError) return null;
    throw error;
  }
}

/**
 * A certificate password as a profile stores it. Tizen Studio writes either the
 * password itself or the path of a `.pwd` file; on macOS that file is never
 * written, and the password is in the login keychain under that path.
 */
export async function profilePassword(
  stored: string,
  platform: NodeJS.Platform = process.platform,
): Promise<string | null> {
  if (!stored.endsWith('.pwd')) return stored === '' ? null : stored;
  if (existsSync(stored)) {
    const parsed = passwordSchema.safeParse(readFileSync(stored, 'utf8').trim());
    return parsed.success ? parsed.data : null;
  }
  return platform === 'darwin' ? fromKeychain(stored) : null;
}
