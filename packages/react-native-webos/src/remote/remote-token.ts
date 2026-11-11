import { chmodSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import { stateDir } from '../state-dir.ts';

const TOKEN = /^[0-9A-Za-z]{1,64}$/;
const PRIVATE_DIR = 0o700;
const PRIVATE_FILE = 0o600;

/** True for a token the TV could have issued; anything else is never stored or sent. */
export function isRemoteToken(value: string): boolean {
  return TOKEN.test(value);
}

/** Where the token a TV issued to `rn-tizen` is kept, readable by this user only. */
export function tokenPath(host: string, env: NodeJS.ProcessEnv = process.env): string {
  return join(stateDir(env), 'remote', `${host}.token`);
}

function cachedTokenPath(host: string, env: NodeJS.ProcessEnv): string {
  return join(cacheDir(env), 'remote', `${host}.token`);
}

function readTokenFile(path: string): string | null {
  try {
    const token = readFileSync(path, 'utf8').trim();
    return isRemoteToken(token) ? token : null;
  } catch {
    return null;
  }
}

/**
 * The saved token for the TV at `host`, or null when it never paired or the file
 * is not a token. A token an older `rn-tizen` kept in the cache moves here.
 */
export function readToken(host: string, env: NodeJS.ProcessEnv = process.env): string | null {
  const token = readTokenFile(tokenPath(host, env));
  if (token !== null) return token;
  const cached = cachedTokenPath(host, env);
  const moved = readTokenFile(cached);
  if (moved === null) return null;
  saveToken(host, moved, env);
  rmSync(cached, { force: true });
  return moved;
}

/** Saves the token the TV at `host` issued, mode 0600 in a 0700 directory. */
export function saveToken(host: string, token: string, env: NodeJS.ProcessEnv = process.env): void {
  const path = tokenPath(host, env);
  const dir = dirname(path);
  mkdirSync(dir, { recursive: true, mode: PRIVATE_DIR });
  chmodSync(dir, PRIVATE_DIR);
  writeFileSync(path, token, { mode: PRIVATE_FILE });
  chmodSync(path, PRIVATE_FILE);
}
