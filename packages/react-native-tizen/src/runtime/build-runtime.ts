import { existsSync } from 'node:fs';
import { join } from 'node:path';
import type { AppPackages, ResolvedPackage } from '../app/app-packages.ts';
import { cacheDir } from '../cache-dir.ts';
import { generateReactNativeSpecs } from '../codegen/react-native-specs.ts';
import { RUNTIME_DIR } from '../package-paths.ts';
import { stampAt } from '../stamp.ts';
import type { Task } from '../task.ts';
import type { TizenSdk } from '../tizen/tizen-sdk.ts';
import type { TizenTarget } from '../tizen/target.ts';
import { buildCMakeProject, sourcesKey, tizenDefinitions } from './cmake.ts';
import type { HermesHostCompilers } from './hermes-host.ts';

/** The native library the .NET shell loads. */
export const RUNTIME_LIBRARY = 'libreact-native-tizen.so';

interface RuntimeBuildDirs {
  readonly root: string;
  readonly codegen: string;
  readonly nativeLibraries: string;
  readonly cmake: string;
  readonly library: string;
}

/**
 * The build directories of the runtime for an app's react-native and a target,
 * one set per runtime sources and react-native on disk, since a CMake build
 * directory belongs to one source tree.
 */
export function runtimeBuildDirs(
  reactNative: ResolvedPackage,
  target: TizenTarget,
  runtimeDir: string = RUNTIME_DIR,
): RuntimeBuildDirs {
  const key = `${reactNative.name}-${reactNative.version}-${target.arch}-tizen${target.platformVersion}-${sourcesKey(runtimeDir, reactNative.dir)}`;
  const root = join(cacheDir(), 'runtime', key);
  const cmake = join(root, 'build');
  return {
    root,
    codegen: join(root, 'codegen'),
    nativeLibraries: join(root, 'libraries'),
    cmake,
    library: join(cmake, 'host', RUNTIME_LIBRARY),
  };
}

/**
 * Runs React Native's codegen into `dir` unless it already holds the output for
 * this exact react-native. Returns whether it ran.
 */
export async function ensureCodegen(reactNative: ResolvedPackage, dir: string): Promise<boolean> {
  const stamp = stampAt(join(dir, '.rn-tizen-codegen.json'));
  const fingerprint = JSON.stringify({ dir: reactNative.dir, version: reactNative.version });
  if (await stamp.matches(fingerprint)) return false;
  await generateReactNativeSpecs(reactNative.dir, dir);
  await stamp.record(fingerprint);
  return true;
}

interface RuntimeInputs {
  readonly sdk: TizenSdk;
  readonly target: TizenTarget;
  readonly packages: AppPackages;
  readonly hermes: HermesHostCompilers;
  readonly dirs: RuntimeBuildDirs;
  readonly nativeLibrariesFile: string;
}

/**
 * Compiles `libreact-native-tizen.so` against the app's own react-native and
 * expo-modules-core, with the app's native libraries. Incremental: an unchanged
 * build only checks timestamps.
 */
export async function compileRuntime(task: Task, inputs: RuntimeInputs): Promise<string> {
  const { packages, dirs } = inputs;
  await buildCMakeProject(task, {
    source: RUNTIME_DIR,
    build: dirs.cmake,
    definitions: {
      ...tizenDefinitions(inputs.sdk, inputs.target),
      REACT_NATIVE_DIR: packages.reactNative.dir,
      RN_TIZEN_CODEGEN_DIR: dirs.codegen,
      RN_TIZEN_HERMES_HOST_COMPILERS: inputs.hermes.importFile,
      EXPO_MODULES_CORE_DIR: packages.expoModulesCore.dir,
      EXPO_MODULES_JSI_DIR: packages.expoModulesJsi.dir,
      RN_TIZEN_LIBRARIES_FILE: inputs.nativeLibrariesFile,
    },
    targets: ['react-native-tizen'],
  });
  if (!existsSync(dirs.library)) throw new Error(`The runtime build produced no ${dirs.library}`);
  return dirs.library;
}
