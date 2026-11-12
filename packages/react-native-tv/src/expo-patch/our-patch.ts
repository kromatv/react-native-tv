import { EXPO_CLI_EXTENSION } from './expo-cli-hook.ts';
import { readText } from './read-text.ts';

const MAX_PATCH_BYTES = 1024 * 1024;

/** Whether the patch file at `file` adds this package's hook, which makes it one `rn-tv expo-patch` may remove. */
export function isOurPatch(file: string): boolean {
  return readText(file, MAX_PATCH_BYTES)?.includes(EXPO_CLI_EXTENSION) ?? false;
}
