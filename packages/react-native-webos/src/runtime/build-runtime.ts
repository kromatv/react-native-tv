import { existsSync } from 'node:fs';
import { join } from 'node:path';
import type { AppPackages, ResolvedPackage } from '../app/app-packages.ts';
import { FixableError } from '../errors.ts';
import { cacheDir } from '../cache-dir.ts';
import { generateReactNativeSpecs } from '../codegen/react-native-specs.ts';
import { RUNTIME_DIR } from '../package-paths.ts';
import { stampAt } from '../stamp.ts';
import type { Task } from '../task.ts';
import type { WebOSSdk } from '../sysroot/webos-sdk.ts';
import { buildCMakeProject, sourcesKey, webosDefinitions } from './cmake.ts';
import type { HermesHostCompilers } from './hermes-host.ts';
import { locateLlvmDir } from './llvm-dir.ts';

/** The executable a package's `appinfo.json` names as `main`: Hermes, React Native, the host, Skia and SDL in one file. */
export const RUNTIME_EXECUTABLE = 'react-native-webos';

/**
 * Which runtime a package gets: a development build's talks to a dev server, a
 * release build's carries none of that (docs/decisions.md).
 */
export type RuntimeFlavour = 'development' | 'release';

interface RuntimeBuildDirs {
  readonly root: string;
  readonly codegen: string;
  readonly nativeLibraries: string;
  readonly cmake: string;
  readonly executable: string;
}

/**
 * The build directories of the TV runtime for an app's react-native and a
 * flavour, one set per runtime sources and react-native on disk, since a CMake
 * build directory belongs to one source tree.
 */
export function runtimeBuildDirs(
  reactNative: ResolvedPackage,
  flavour: RuntimeFlavour,
  runtimeDir: string = RUNTIME_DIR,
): RuntimeBuildDirs {
  const key = `${reactNative.name}-${reactNative.version}-${flavour}-${sourcesKey(runtimeDir, reactNative.dir)}`;
  const root = join(cacheDir(), 'runtime', key);
  const cmake = join(root, 'build');
  return {
    root,
    codegen: join(root, 'codegen'),
    nativeLibraries: join(root, 'libraries'),
    cmake,
    executable: join(cmake, 'host', RUNTIME_EXECUTABLE),
  };
}

/**
 * Runs React Native's codegen into `dir` unless it already holds the output for
 * this exact react-native. Returns whether it ran.
 */
export async function ensureCodegen(reactNative: ResolvedPackage, dir: string): Promise<boolean> {
  const stamp = stampAt(join(dir, '.rn-webos-codegen.json'));
  const fingerprint = JSON.stringify({ dir: reactNative.dir, version: reactNative.version });
  if (await stamp.matches(fingerprint)) return false;
  await generateReactNativeSpecs(reactNative.dir, dir);
  await stamp.record(fingerprint);
  return true;
}

interface RuntimeInputs {
  readonly sdk: WebOSSdk;
  readonly flavour: RuntimeFlavour;
  readonly packages: AppPackages;
  readonly hermes: HermesHostCompilers;
  readonly dirs: RuntimeBuildDirs;
  readonly nativeLibrariesFile: string;
}

/**
 * Compiles the `react-native-webos` executable against the app's own
 * react-native and expo-modules-core, with the app's native libraries, in the
 * given flavour. Incremental: an unchanged build only checks timestamps.
 */
export async function compileRuntime(task: Task, inputs: RuntimeInputs): Promise<string> {
  const { packages, dirs } = inputs;
  await buildCMakeProject(task, {
    source: RUNTIME_DIR,
    build: dirs.cmake,
    definitions: {
      ...webosDefinitions(inputs.sdk, locateLlvmDir()),
      REACT_NATIVE_DIR: packages.reactNative.dir,
      RN_WEBOS_CODEGEN_DIR: dirs.codegen,
      RN_WEBOS_HERMES_HOST_COMPILERS: inputs.hermes.importFile,
      EXPO_MODULES_CORE_DIR: packages.expoModulesCore.dir,
      EXPO_MODULES_JSI_DIR: packages.expoModulesJsi.dir,
      RN_WEBOS_LIBRARIES_FILE: inputs.nativeLibrariesFile,
      RN_WEBOS_DEVELOPMENT: inputs.flavour === 'development' ? 'ON' : 'OFF',
    },
    targets: [RUNTIME_EXECUTABLE],
  });
  if (!existsSync(dirs.executable)) {
    throw new FixableError(
      `The runtime build produced no ${dirs.executable}`,
      'Run again with --verbose to see what CMake and Ninja printed.',
    );
  }
  return dirs.executable;
}
