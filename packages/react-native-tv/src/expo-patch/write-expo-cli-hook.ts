import { readFileSync, renameSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { addExpoCliHook, hasExpoCliHook } from './expo-cli-hook.ts';

/**
 * Adds the hook to the Expo CLI entry at `file` unless it has it, and says
 * whether it has it now: false is an entry laid out in a way the hook does not
 * know, left as it was.
 */
export function writeExpoCliHook(file: string): boolean {
  const source = readFileSync(file, 'utf8');
  if (hasExpoCliHook(source)) return true;
  const patched = addExpoCliHook(source);
  if (patched === null) return false;
  // A package manager can hard-link an installed file to its store: writing a new
  // file and renaming it over the old one leaves every other link as it was.
  const replacement = `${file}.rn-tv-expo-patch`;
  try {
    writeFileSync(replacement, patched, { mode: statSync(file).mode });
    renameSync(replacement, file);
  } catch (error) {
    rmSync(replacement, { recursive: true, force: true });
    throw error;
  }
  return true;
}
