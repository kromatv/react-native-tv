import { createHash } from 'node:crypto';
import {
  mkdir,
  readFile,
  readdir,
  readlink,
  rename,
  rm,
  symlink,
  writeFile,
} from 'node:fs/promises';
import { basename, dirname, join, posix } from 'node:path';
import { z } from 'zod';
import { isElfSharedObject, rebindStrayLocalSymbols } from './elf-dynamic-symbols.ts';

const MIRROR_FORMAT = 1;
const STAMP_FILE = '.rn-tizen-sysroot.json';
const LIBRARY_DIRS = ['lib', 'lib64', 'usr/lib', 'usr/lib64'] as const;
const COMPAT_ARCHIVE = 'librn-tizen-glibc-compat.a';
const LIBC_NONSHARED = /(\/usr\/lib(?:64)?\/)libc_nonshared\.a/;

const stampSchema = z.object({
  format: z.literal(MIRROR_FORMAT),
  rootstrap: z.string(),
  compatDigest: z.string(),
});

/** What {@link mirrorSysroot} built, or found already built. */
export interface MirroredSysroot {
  readonly path: string;
  readonly patchedLibraries: number;
  readonly reused: boolean;
}

interface MirrorRequest {
  readonly rootstrap: string;
  readonly destination: string;
  readonly compatArchive: string;
}

function isLibraryDir(relative: string): boolean {
  return (LIBRARY_DIRS as readonly string[]).includes(relative);
}

function leadsToLibraryDir(relative: string): boolean {
  return LIBRARY_DIRS.some((dir) => dir.startsWith(`${relative}/`));
}

async function readStamp(destination: string): Promise<z.infer<typeof stampSchema> | null> {
  try {
    const raw: unknown = JSON.parse(await readFile(join(destination, STAMP_FILE), 'utf8'));
    const parsed = stampSchema.safeParse(raw);
    return parsed.success ? parsed.data : null;
  } catch {
    return null;
  }
}

function withCompatArchive(linkerScript: string): string {
  return linkerScript.replace(
    LIBC_NONSHARED,
    (match, dir: string) => `${match} ${dir}${COMPAT_ARCHIVE}`,
  );
}

async function mirrorLibraryFile(
  source: string,
  target: string,
  compatArchive: string,
): Promise<boolean> {
  const bytes = new Uint8Array(await readFile(source));
  if (basename(source) === 'libc.so' && !isElfSharedObject(bytes)) {
    await writeFile(target, withCompatArchive(new TextDecoder().decode(bytes)));
    await writeFile(join(dirname(target), COMPAT_ARCHIVE), await readFile(compatArchive));
    return false;
  }
  if (isElfSharedObject(bytes) && rebindStrayLocalSymbols(bytes) > 0) {
    await writeFile(target, bytes);
    return true;
  }
  await symlink(source, target);
  return false;
}

async function mirrorLibraryDir(
  source: string,
  target: string,
  compatArchive: string,
): Promise<number> {
  await mkdir(target, { recursive: true });
  let patched = 0;
  for (const entry of await readdir(source, { withFileTypes: true })) {
    const from = join(source, entry.name);
    const to = join(target, entry.name);
    if (entry.isSymbolicLink()) await symlink(await readlink(from), to);
    else if (entry.isDirectory()) await symlink(from, to);
    else if (await mirrorLibraryFile(from, to, compatArchive)) patched += 1;
  }
  return patched;
}

async function mirrorDir(
  request: MirrorRequest,
  staging: string,
  relative: string,
): Promise<number> {
  const source = join(request.rootstrap, relative);
  const target = join(staging, relative);
  if (isLibraryDir(relative)) return mirrorLibraryDir(source, target, request.compatArchive);
  await mkdir(target, { recursive: true });
  let patched = 0;
  for (const entry of await readdir(source, { withFileTypes: true })) {
    const child = relative === '' ? entry.name : posix.join(relative, entry.name);
    if (entry.isDirectory() && (isLibraryDir(child) || leadsToLibraryDir(child))) {
      patched += await mirrorDir(request, staging, child);
    } else {
      await symlink(join(source, entry.name), join(target, entry.name));
    }
  }
  return patched;
}

/**
 * Mirrors a Tizen rootstrap into `destination` so lld can link against it:
 * every file is a symlink to the original except the shared libraries whose
 * symbol tables need {@link rebindStrayLocalSymbols}, which are patched copies,
 * and `libc.so`, whose linker script also pulls in `compatArchive`. Rebuilds only
 * when the rootstrap or the archive changed.
 */
export async function mirrorSysroot(request: MirrorRequest): Promise<MirroredSysroot> {
  const compatDigest = createHash('sha256')
    .update(await readFile(request.compatArchive))
    .digest('hex');
  const stamp = await readStamp(request.destination);
  if (stamp?.rootstrap === request.rootstrap && stamp.compatDigest === compatDigest) {
    return { path: request.destination, patchedLibraries: 0, reused: true };
  }

  const staging = `${request.destination}.staging-${process.pid}`;
  await rm(staging, { recursive: true, force: true });
  const patchedLibraries = await mirrorDir(request, staging, '');
  const nextStamp = { format: MIRROR_FORMAT, rootstrap: request.rootstrap, compatDigest };
  await writeFile(join(staging, STAMP_FILE), JSON.stringify(nextStamp, null, 2));

  await mkdir(dirname(request.destination), { recursive: true });
  await rm(request.destination, { recursive: true, force: true });
  await rename(staging, request.destination);
  return { path: request.destination, patchedLibraries, reused: false };
}
