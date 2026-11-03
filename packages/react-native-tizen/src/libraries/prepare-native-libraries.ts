import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import type { ResolvedPackage } from '../app/app-packages.ts';
import { type Codegen, loadCodegen } from '../codegen/codegen.ts';
import { generateLibrarySpecs } from '../codegen/library-specs.ts';
import { stampAt } from '../stamp.ts';
import { bundledImplementations } from './bundled-implementations.ts';
import { findNativeLibraries, type NativeLibrary } from './find-native-libraries.ts';

const GENERATOR_VERSION = 1;

/** A native library as the runtime build compiles it. */
export interface LinkedLibrary {
  readonly name: string;
  readonly version: string;
  readonly dir: string;
  readonly codegenName: string;
  readonly codegenDir: string;
  readonly implementation: string | null;
}

/** The app's native libraries, generated and listed for `runtime/cmake/Libraries.cmake`. */
export interface PreparedLibraries {
  readonly file: string;
  readonly libraries: readonly LinkedLibrary[];
  readonly warnings: readonly string[];
}

async function writeIfChanged(path: string, contents: string): Promise<void> {
  const current = await readFile(path, 'utf8').catch(() => null);
  if (current !== contents) await writeFile(path, contents);
}

async function ensureLibraryCodegen(
  codegen: () => Codegen,
  library: NativeLibrary,
  reactNative: ResolvedPackage,
  outputDir: string,
): Promise<void> {
  if (library.codegen === null) {
    await mkdir(outputDir, { recursive: true });
    return;
  }
  const stamp = stampAt(join(outputDir, '.rn-tizen-codegen.json'));
  const fingerprint = JSON.stringify({
    generator: GENERATOR_VERSION,
    reactNative: [reactNative.dir, reactNative.version],
    library: [library.dir, library.version, library.codegen],
  });
  if (await stamp.matches(fingerprint)) return;
  const specsDir = join(library.dir, library.codegen.jsSrcsDir);
  await generateLibrarySpecs(codegen(), library.codegen.name, specsDir, outputDir);
  await stamp.record(fingerprint);
}

/**
 * Finds the app's packages that ship codegen specs and its Expo modules that
 * support Tizen, runs codegen for each one with specs that changed into `dir`,
 * picks each one's Tizen implementation (its own, else one this package
 * bundles, else none) and writes `dir/libraries.json`, rewritten only when it
 * changes so CMake reconfigures only then.
 */
export async function prepareNativeLibraries(
  appRoot: string,
  reactNative: ResolvedPackage,
  dir: string,
): Promise<PreparedLibraries> {
  const scan = findNativeLibraries(appRoot, [reactNative.dir]);
  const bundled = bundledImplementations();
  let codegen: Codegen | undefined;
  const loaded = () => (codegen ??= loadCodegen(reactNative.dir));
  const libraries: LinkedLibrary[] = [];
  for (const library of scan.libraries) {
    const codegenDir = join(dir, library.targetName);
    await ensureLibraryCodegen(loaded, library, reactNative, codegenDir);
    libraries.push({
      name: library.name,
      version: library.version,
      dir: library.dir,
      codegenName: library.targetName,
      codegenDir,
      implementation: library.declaredTizenCMake ?? bundled.get(library.name) ?? null,
    });
  }
  await mkdir(dir, { recursive: true });
  const file = join(dir, 'libraries.json');
  await writeIfChanged(file, `${JSON.stringify({ libraries }, null, 2)}\n`);
  return { file, libraries, warnings: scan.warnings };
}

/** One line naming each library and where its Tizen implementation comes from. */
export function describeLibraries(libraries: readonly LinkedLibrary[]): string {
  if (libraries.length === 0) return 'none';
  return libraries
    .map((library) => `${library.name}${library.implementation === null ? ' (codegen only)' : ''}`)
    .join(', ');
}
