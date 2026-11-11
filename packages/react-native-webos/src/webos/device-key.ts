import { chmodSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { stateDir } from '../state-dir.ts';
import type { TvAddress } from './device-address.ts';

const PRIVATE_DIR = 0o700;
const PRIVATE_FILE = 0o600;
const MAX_KEY_CHARS = 65_536;
const PEM_KEY = /-----BEGIN [A-Z ]*PRIVATE KEY-----/;

/** Where the SSH key a TV gave out is kept, readable by this user only. */
export function deviceKeyPath(address: TvAddress, env: NodeJS.ProcessEnv = process.env): string {
  return join(stateDir(env), 'keys', `${address.host}_${address.port}.pem`);
}

/** The key kept for a TV, or null when `rn-webos connect` never fetched one. */
export function readDeviceKey(
  address: TvAddress,
  env: NodeJS.ProcessEnv = process.env,
): string | null {
  try {
    const pem = readFileSync(deviceKeyPath(address, env), 'utf8');
    return pem.length <= MAX_KEY_CHARS && PEM_KEY.test(pem) ? pem : null;
  } catch {
    return null;
  }
}

/** Keeps a TV's decrypted key, mode 0600 in a 0700 directory. */
export function saveDeviceKey(
  address: TvAddress,
  pem: string,
  env: NodeJS.ProcessEnv = process.env,
): void {
  const path = deviceKeyPath(address, env);
  const dir = dirname(path);
  mkdirSync(dir, { recursive: true, mode: PRIVATE_DIR });
  chmodSync(dir, PRIVATE_DIR);
  writeFileSync(path, pem, { mode: PRIVATE_FILE });
  chmodSync(path, PRIVATE_FILE);
}
