import { homedir } from 'node:os';
import { join } from 'node:path';

/**
 * Where downloads, the SDK and runtime builds live. Shared by every
 * project on the machine; `RN_WEBOS_CACHE_DIR` overrides it, and the CMake
 * toolchain resolves the same directory.
 */
export function cacheDir(env: NodeJS.ProcessEnv = process.env): string {
  if (env.RN_WEBOS_CACHE_DIR) return env.RN_WEBOS_CACHE_DIR;
  if (env.XDG_CACHE_HOME) return join(env.XDG_CACHE_HOME, 'react-native-webos');
  return join(homedir(), '.cache', 'react-native-webos');
}
