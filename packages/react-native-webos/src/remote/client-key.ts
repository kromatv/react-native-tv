import { chmodSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { stateDir } from '../state-dir.ts';

const CLIENT_KEY = /^[0-9A-Za-z]{1,128}$/;
const TV_ID = /^[0-9A-Za-z][0-9A-Za-z.-]{0,63}$/;
const PRIVATE_DIR = 0o700;
const PRIVATE_FILE = 0o600;

/** True for a client key a TV could have issued; anything else is never stored or sent. */
export function isClientKey(value: string): boolean {
  return CLIENT_KEY.test(value);
}

/** Where the client key a TV issued to `rn-webos` is kept, by the TV's UUID, else its address. */
export function clientKeyPath(tvId: string, env: NodeJS.ProcessEnv = process.env): string {
  if (!TV_ID.test(tvId)) throw new Error(`${JSON.stringify(tvId)} cannot name a TV's client key`);
  return join(stateDir(env), 'remote', `${tvId}.key`);
}

/** The client key saved for a TV, or null when it never paired or the file holds no key. */
export function readClientKey(tvId: string, env: NodeJS.ProcessEnv = process.env): string | null {
  try {
    const key = readFileSync(clientKeyPath(tvId, env), 'utf8').trim();
    return isClientKey(key) ? key : null;
  } catch {
    return null;
  }
}

/** Saves the client key a TV issued, mode 0600 in a 0700 directory. */
export function saveClientKey(
  tvId: string,
  key: string,
  env: NodeJS.ProcessEnv = process.env,
): void {
  const path = clientKeyPath(tvId, env);
  const dir = dirname(path);
  mkdirSync(dir, { recursive: true, mode: PRIVATE_DIR });
  chmodSync(dir, PRIVATE_DIR);
  writeFileSync(path, key, { mode: PRIVATE_FILE });
  chmodSync(path, PRIVATE_FILE);
}
