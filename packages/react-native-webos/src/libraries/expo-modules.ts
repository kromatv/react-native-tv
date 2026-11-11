import { existsSync, readdirSync, readFileSync, realpathSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { WEBOS_PLATFORM } from '../metro/with-webos.ts';
import {
  autolinkingPackageSchema,
  expoModuleConfigSchema,
  webosDeclarationSchema,
} from './library-package.ts';

/** The file an Expo module is recognised by, at its root. */
export const EXPO_MODULE_CONFIG = 'expo-module.config.json';

const DEFAULT_NATIVE_MODULES_DIR = 'modules';

function readJson(path: string): unknown {
  try {
    return JSON.parse(readFileSync(path, 'utf8'));
  } catch {
    return null;
  }
}

/**
 * The webOS CMakeLists.txt the Expo module at `dir` declares, as an absolute
 * path: its `expo-module.config.json` lists `webos` in `platforms` and names
 * the file in `webos.cmake`. Null for a directory that is not an Expo module
 * or does not support webOS; a module that lists `webos` without a `cmake`
 * adds a warning.
 */
export function webosExpoModuleCMake(dir: string, name: string, warnings: string[]): string | null {
  const configPath = join(dir, EXPO_MODULE_CONFIG);
  if (!existsSync(configPath)) return null;
  const config = expoModuleConfigSchema.safeParse(readJson(configPath));
  if (!config.success || config.data.platforms?.includes(WEBOS_PLATFORM) !== true) return null;
  const declaration = webosDeclarationSchema.safeParse(config.data.webos);
  if (declaration.success) return join(dir, declaration.data.cmake);
  warnings.push(
    `${name}: its ${EXPO_MODULE_CONFIG} lists webos but no "webos": { "cmake" }; ignored`,
  );
  return null;
}

function nativeModulesDir(appRoot: string): string {
  const parsed = autolinkingPackageSchema.safeParse(readJson(join(appRoot, 'package.json')));
  const options = parsed.success ? parsed.data.expo?.autolinking : undefined;
  const dir =
    options?.webos?.nativeModulesDir ?? options?.nativeModulesDir ?? DEFAULT_NATIVE_MODULES_DIR;
  return resolve(appRoot, dir);
}

/**
 * The app's local Expo modules, as Expo's autolinking finds them: each
 * directory of `expo.autolinking.nativeModulesDir` (default `modules`) that
 * holds an `expo-module.config.json`, by real path and directory name.
 */
export function localExpoModules(
  appRoot: string,
): readonly { readonly dir: string; readonly name: string }[] {
  const root = nativeModulesDir(appRoot);
  if (!existsSync(root)) return [];
  return readdirSync(root, { withFileTypes: true })
    .filter(
      (entry) => entry.isDirectory() && existsSync(join(root, entry.name, EXPO_MODULE_CONFIG)),
    )
    .map((entry) => ({ dir: realpathSync(join(root, entry.name)), name: entry.name }))
    .sort((left, right) => left.name.localeCompare(right.name));
}

/** The CMake target name an Expo module's webOS implementation is built under. */
export function expoModuleTargetName(name: string): string {
  return `expo_${name.replace(/^@/, '').replace(/\W/g, '_')}`;
}
