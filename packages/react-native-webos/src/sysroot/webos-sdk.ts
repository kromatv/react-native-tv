import { existsSync } from 'node:fs';
import { mkdir, rename, rm } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import { downloadVerified } from '../runtime/download.ts';
import { stampAt } from '../stamp.ts';
import type { Task } from '../task.ts';
import { SDK_DIRECTORIES, SDK_EXCLUDED, SDK_PIN } from './sdk-pin.ts';

/** The extracted SDK a TV build compiles and links against. */
export interface WebOSSdk {
  /** Holds `arm-webos-linux-gnueabi/sysroot` and `lib/gcc`, as the toolchain file expects. */
  readonly path: string;
  readonly version: string;
  /** False when this call downloaded or extracted it. */
  readonly reused: boolean;
}

const STAMP_FILE = '.rn-webos-sdk.json';
const FINGERPRINT = JSON.stringify({
  format: 1,
  sha256: SDK_PIN.sha256,
  directories: SDK_DIRECTORIES,
  excluded: SDK_EXCLUDED,
});

function sdkPath(): string {
  return join(cacheDir(), 'sdk', SDK_PIN.version);
}

function archivePath(): string {
  const file = new URL(SDK_PIN.url).pathname.split('/').at(-1) ?? 'sdk.tar.gz';
  return join(cacheDir(), 'downloads', 'webos-sdk', `webos-sdk-${file}`);
}

/** The SDK in the cache, or null when `rn-webos setup` has not extracted this version yet. */
export async function installedWebOSSdk(): Promise<WebOSSdk | null> {
  const path = sdkPath();
  if (!(await stampAt(join(path, STAMP_FILE)).matches(FINGERPRINT))) return null;
  return { path, version: SDK_PIN.version, reused: true };
}

async function extract(task: Task, archive: string, path: string): Promise<void> {
  const staging = `${path}.staging-${process.pid}`;
  await rm(staging, { recursive: true, force: true });
  await mkdir(staging, { recursive: true });
  const inArchive = (member: string) => `${SDK_PIN.root}/${member}`;
  await task.run([
    'tar',
    '-xzf',
    archive,
    '-C',
    staging,
    '--strip-components=1',
    ...SDK_EXCLUDED.map((member) => `--exclude=${inArchive(member)}`),
    ...SDK_DIRECTORIES.map(inArchive),
  ]);
  const missing = SDK_DIRECTORIES.find((directory) => !existsSync(join(staging, directory)));
  if (missing !== undefined) throw new Error(`The SDK archive held no ${missing}`);
  await stampAt(join(staging, STAMP_FILE)).record(FINGERPRINT);
  await rm(path, { recursive: true, force: true });
  await mkdir(dirname(path), { recursive: true });
  await rename(staging, path);
}

/**
 * Makes a TV build possible: downloads the pinned SDK once, checks its digest
 * and extracts the directories a build reads into the cache. Cheap to repeat.
 */
export async function ensureWebOSSdk(task: Task): Promise<WebOSSdk> {
  const installed = await installedWebOSSdk();
  if (installed !== null) return installed;
  const archive = archivePath();
  task.progress('downloading');
  await downloadVerified(SDK_PIN.url, SDK_PIN.sha256, archive);
  task.progress('extracting');
  const path = sdkPath();
  try {
    await extract(task, archive, path);
  } finally {
    await rm(`${path}.staging-${process.pid}`, { recursive: true, force: true });
  }
  return { path, version: SDK_PIN.version, reused: false };
}
