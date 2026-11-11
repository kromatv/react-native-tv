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
import { ensureWebOSSdk } from '../sysroot/webos-sdk.ts';

/**
 * Prepares the SDK, the Hermes host compilers, React Native's codegen and the
 * app's native libraries, then compiles the TV runtime of `flavour`. Every
 * step is skipped or incremental when its inputs did not change. Returns the
 * executable's path.
 */
export async function runtimeSteps(
  reporter: Reporter,
  app: AppProject,
  flavour: RuntimeFlavour,
): Promise<string> {
  const sdk = await reporter.step(
    'Toolchain',
    (task) => ensureWebOSSdk(task),
    (found) => (found.reused ? 'cached' : `SDK ${found.version} extracted`),
  );
  const cachedHermes = hermesHostCompilers();
  const hermes = await reporter.step(
    'Hermes compilers',
    (task) => ensureHermesHostCompilers(task),
    (built) => (cachedHermes.built ? 'cached' : `hermes ${built.version}`),
  );
  const dirs = runtimeBuildDirs(app.packages.reactNative, flavour);
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
        flavour,
        packages: app.packages,
        hermes,
        dirs,
        nativeLibrariesFile: nativeLibraries.file,
      }),
    (executable) => `${flavour}, ${formatSize(statSync(executable).size)}`,
  );
}
