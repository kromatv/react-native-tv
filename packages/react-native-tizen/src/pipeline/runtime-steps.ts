import { statSync } from 'node:fs';
import type { AppProject } from '../app/app-project.ts';
import { formatSize } from '../cli/format.ts';
import type { Reporter } from '../cli/reporter.ts';
import {
  describeLibraries,
  prepareNativeLibraries,
} from '../libraries/prepare-native-libraries.ts';
import {
  compileRuntime,
  ensureCodegen,
  runtimeBuildDirs,
  type RuntimeFlavour,
} from '../runtime/build-runtime.ts';
import { ensureHermesHostCompilers, hermesHostCompilers } from '../runtime/hermes-host.ts';
import { setupTarget } from '../sysroot/setup-target.ts';
import type { TizenSdk } from '../tizen/tizen-sdk.ts';
import type { TizenTarget } from '../tizen/target.ts';

/**
 * Prepares the target, the Hermes host compilers, React Native's codegen and
 * the app's native libraries, then compiles the native runtime of `flavour`.
 * Every step is skipped or incremental when its inputs did not change.
 * Returns the library's path.
 */
export async function runtimeSteps(
  reporter: Reporter,
  sdk: TizenSdk,
  app: AppProject,
  target: TizenTarget,
  flavour: RuntimeFlavour,
): Promise<string> {
  await reporter.step(
    'Toolchain',
    (task) => setupTarget(task, sdk, target),
    (sysroot) =>
      sysroot.reused ? 'cached' : `sysroot mirrored, ${sysroot.patchedLibraries} libraries patched`,
  );
  const cachedHermes = hermesHostCompilers();
  const hermes = await reporter.step(
    'Hermes compilers',
    (task) => ensureHermesHostCompilers(task),
    (built) => (cachedHermes.built ? 'cached' : `hermes ${built.version}`),
  );
  const dirs = runtimeBuildDirs(app.packages.reactNative, target, flavour);
  await reporter.step(
    'React Native codegen',
    () => ensureCodegen(app.packages.reactNative, dirs.codegen),
    (ran) =>
      ran ? `${app.packages.reactNative.name} ${app.packages.reactNative.version}` : 'cached',
  );
  const nativeLibraries = await reporter.step(
    'Native libraries',
    () => prepareNativeLibraries(app.root, app.packages.reactNative, dirs.nativeLibraries),
    (prepared) => describeLibraries(prepared.libraries),
  );
  for (const warning of nativeLibraries.warnings) reporter.note(warning);
  return reporter.step(
    'Native runtime',
    (task) =>
      compileRuntime(task, {
        sdk,
        target,
        flavour,
        packages: app.packages,
        hermes,
        dirs,
        nativeLibrariesFile: nativeLibraries.file,
      }),
    (library) => `${flavour}, ${formatSize(statSync(library).size)}`,
  );
}
