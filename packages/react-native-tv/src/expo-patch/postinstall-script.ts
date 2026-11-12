import { writeFileSync } from 'node:fs';
import { readManifest } from './app-manifest.ts';
import { setJsonString } from './edit-json.ts';

/** What the app's `postinstall` runs, for a package manager with no patch feature. */
export const POSTINSTALL_COMMAND = 'rn-tv expo-patch';

/**
 * Makes the `postinstall` script of the `package.json` at `file` run
 * `rn-tv expo-patch`, after what it runs already, and says whether the file
 * changed: a manifest with the command in any of its scripts is left alone.
 */
export function addPostinstallScript(file: string): boolean {
  const manifest = readManifest(file);
  if (manifest === null) throw new Error(`${file} is not a package.json rn-tv can read.`);
  const scripts = Object.values(manifest.scripts);
  if (scripts.some((script) => script.includes(POSTINSTALL_COMMAND))) return false;
  const before = manifest.scripts.postinstall?.trim() ?? '';
  const script = before === '' ? POSTINSTALL_COMMAND : `${before} && ${POSTINSTALL_COMMAND}`;
  const edited = setJsonString(manifest.text, 'scripts', 'postinstall', script);
  if (edited === null) throw new Error(`${file} has scripts rn-tv cannot add to.`);
  writeFileSync(file, edited);
  return true;
}
