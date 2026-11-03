import { existsSync, readdirSync, readFileSync, realpathSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { TIZEN_PLATFORM } from '../metro/with-tizen.ts';
import {
  autolinkingPackageSchema,
  expoModuleConfigSchema,
  tizenDeclarationSchema,
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
 * The Tizen CMakeLists.txt the Expo module at `dir` declares, as an absolute
 * path: its `expo-module.config.json` lists `tizen` in `platforms` and names
 * the file in `tizen.cmake`. Null for a directory that is not an Expo module
 * or does not support Tizen; a module that lists `tizen` without a `cmake`
 * adds a warning.
 */
export function tizenExpoModuleCMake(dir: string, name: string, warnings: string[]): string | null {
  const configPath = join(dir, EXPO_MODULE_CONFIG);
  if (!existsSync(configPath)) return null;
  const config = expoModuleConfigSchema.safeParse(readJson(configPath));
  if (!config.success || config.data.platforms?.includes(TIZEN_PLATFORM) !== true) return null;
  const declaration = tizenDeclarationSchema.safeParse(config.data.tizen);
  if (declaration.success) return join(dir, declaration.data.cmake);
  warnings.push(
    `${name}: its ${EXPO_MODULE_CONFIG} lists tizen but no "tizen": { "cmake" }; ignored`,
  );
  return null;
}

function nativeModulesDir(appRoot: string): string {
  const parsed = autolinkingPackageSchema.safeParse(readJson(join(appRoot, 'package.json')));
  const options = parsed.success ? parsed.data.expo?.autolinking : undefined;
  const dir =
    options?.tizen?.nativeModulesDir ?? options?.nativeModulesDir ?? DEFAULT_NATIVE_MODULES_DIR;
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

/** The CMake target name an Expo module's Tizen implementation is built under. */
export function expoModuleTargetName(name: string): string {
  return `expo_${name.replace(/^@/, '').replace(/[^A-Za-z0-9_]/g, '_')}`;
}
