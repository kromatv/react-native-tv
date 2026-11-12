import { rmdirSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, isAbsolute, join, relative } from 'node:path';
import { readManifest, type AppManifest } from './app-manifest.ts';
import { removeJsonMember } from './edit-json.ts';
import { isOurPatch } from './our-patch.ts';

/** The patch one entry of a `package.json` applies: its file, relative to the install directory. */
export interface PatchEntry {
  readonly version: string;
  readonly file: string;
}

/** The member of `package.json` a package manager keeps its patches in, and how to read one entry. */
export interface PatchEntries {
  readonly member: 'patchedDependencies' | 'resolutions';
  read(key: string, value: string): PatchEntry | null;
}

function removeEmptyDirectory(dir: string): void {
  try {
    rmdirSync(dir);
  } catch {
    // The directory holds other patches.
  }
}

function patchFile(installDir: string, entry: PatchEntry): string | null {
  const file = join(installDir, entry.file);
  const inside = relative(installDir, file);
  return isAbsolute(entry.file) || inside.startsWith('..') ? null : file;
}

function entriesOf(manifest: AppManifest | null, entries: PatchEntries): Map<string, PatchEntry> {
  const found = new Map<string, PatchEntry>();
  for (const [key, value] of Object.entries(manifest?.[entries.member] ?? {})) {
    const entry = entries.read(key, value);
    if (entry !== null) found.set(key, entry);
  }
  return found;
}

/** The patches of Expo CLI the `package.json` in `installDir` names. */
export function patchEntries(installDir: string, entries: PatchEntries): PatchEntry[] {
  return [...entriesOf(readManifest(join(installDir, 'package.json')), entries).values()];
}

/**
 * Removes from the `package.json` in `installDir` each patch of Expo CLI that
 * `isOld` accepts and that is this package's, with its patch file once no
 * entry uses it, and gives the versions removed. A patch that does not add
 * this package's hook is never touched.
 */
export function removeOldPatchEntries(
  installDir: string,
  entries: PatchEntries,
  isOld: (entry: PatchEntry) => boolean,
): string[] {
  const manifestFile = join(installDir, 'package.json');
  const manifest = readManifest(manifestFile);
  const kept = entriesOf(manifest, entries);
  const removed: PatchEntry[] = [];
  let text = manifest?.text ?? '';
  for (const [key, entry] of [...kept]) {
    const file = patchFile(installDir, entry);
    if (file === null || !isOld(entry) || !isOurPatch(file)) continue;
    text = removeJsonMember(text, [entries.member, key]);
    kept.delete(key);
    removed.push(entry);
  }
  if (removed.length === 0) return [];
  writeFileSync(manifestFile, text);
  const inUse = new Set([...kept.values()].map((entry) => entry.file));
  for (const entry of removed) {
    if (inUse.has(entry.file)) continue;
    rmSync(join(installDir, entry.file), { force: true });
    removeEmptyDirectory(dirname(join(installDir, entry.file)));
  }
  return [...new Set(removed.map((entry) => entry.version))];
}
