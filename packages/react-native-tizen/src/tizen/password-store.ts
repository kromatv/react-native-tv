import { spawn } from 'node:child_process';
import { writeFile } from 'node:fs/promises';
import { profilePassword } from './profile-password.ts';

const KEYCHAIN_SERVICE = 'certificate-manager';
const SECURITY = '/usr/bin/security';

function quoted(text: string): string {
  const escaped = text.replaceAll('\\', String.raw`\\`).replaceAll('"', String.raw`\"`);
  return `"${escaped}"`;
}

function addToKeychain(account: string, password: string): Promise<void> {
  return new Promise((resolve, reject) => {
    const child = spawn(SECURITY, ['-i'], { stdio: ['pipe', 'ignore', 'pipe'] });
    let errors = '';
    child.stderr.on('data', (chunk: Buffer) => {
      errors += chunk.toString('utf8').slice(0, 1024);
    });
    child.on('error', reject);
    child.on('close', (code) => {
      const said = errors.trim() === '' ? `exit ${code}` : errors.trim();
      if (code === 0 && errors.trim() === '') resolve();
      else reject(new Error(`security could not store the password: ${said}`));
    });
    child.stdin.end(
      `add-generic-password -U -s ${KEYCHAIN_SERVICE} -a ${quoted(account)} -w ${quoted(password)}\n`,
    );
  });
}

/**
 * Stores a certificate password where Tizen Studio looks for it: on macOS in
 * the login keychain under the `.pwd` path, elsewhere in that file, readable
 * by its owner only. The password goes to `security` over stdin, not argv.
 */
export async function storePassword(
  pwdPath: string,
  password: string,
  platform: NodeJS.Platform = process.platform,
): Promise<void> {
  if (platform === 'darwin') await addToKeychain(pwdPath, password);
  else await writeFile(pwdPath, password, { mode: 0o600 });
  if ((await profilePassword(pwdPath, platform)) !== password) {
    throw new Error(`The password stored for ${pwdPath} does not read back`);
  }
}
