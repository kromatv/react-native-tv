import { existsSync, readFileSync, realpathSync } from 'node:fs';
import { basename, dirname, join, resolve } from 'node:path';
import { expoModuleTargetName, localExpoModules, tizenExpoModuleCMake } from './expo-modules.ts';
import {
  type CodegenConfig,
  codegenConfigSchema,
  type LibraryPackage,
  libraryPackageSchema,
  tizenDeclarationSchema,
} from './library-package.ts';

/**
 * A package of the app that ships React Native codegen specs, or an Expo
 * module, a dependency or one of the app's own, that declares a Tizen
 * implementation.
 */
export interface NativeLibrary {
  readonly name: string;
  readonly version: string;
  readonly dir: string;
  /** The codegen name, or {@link expoModuleTargetName} for an Expo module. */
  readonly targetName: string;
  /** Null for an Expo module, which has no specs. */
  readonly codegen: CodegenConfig | null;
  readonly declaredTizenCMake: string | null;
}

/** What {@link findNativeLibraries} found, and what it had to skip. */
export interface LibraryScan {
  readonly libraries: readonly NativeLibrary[];
  readonly warnings: readonly string[];
}

const MAX_DEPTH = 9;

interface Visit {
  readonly dir: string;
  readonly depth: number;
  readonly fallbackName: string;
}

function readPackage(dir: string): LibraryPackage | null {
  try {
    const parsed = libraryPackageSchema.safeParse(
      JSON.parse(readFileSync(join(dir, 'package.json'), 'utf8')),
    );
    return parsed.success ? parsed.data : null;
  } catch {
    return null;
  }
}

function dependencyNames(manifest: LibraryPackage, depth: number): string[] {
  const optionalPeers = new Set(
    Object.entries(manifest.peerDependenciesMeta ?? {})
      .filter(([, meta]) => meta.optional === true)
      .map(([name]) => name),
  );
  const peers = Object.keys(manifest.peerDependencies ?? {}).filter(
    (name) => !optionalPeers.has(name),
  );
  const own = Object.keys(manifest.dependencies ?? {});
  const dev = depth === 0 ? Object.keys(manifest.devDependencies ?? {}) : [];
  return [...new Set([...own, ...dev, ...peers])];
}

function resolveFrom(fromDir: string, name: string): string | null {
  let current = resolve(fromDir);
  for (;;) {
    if (basename(current) !== 'node_modules') {
      const candidate = join(current, 'node_modules', name);
      if (existsSync(join(candidate, 'package.json'))) return realpathSync(candidate);
    }
    const parent = dirname(current);
    if (parent === current) return null;
    current = parent;
  }
}

function describeExpoModule(
  dir: string,
  name: string,
  version: string,
  warnings: string[],
): NativeLibrary | null {
  const cmake = tizenExpoModuleCMake(dir, name, warnings);
  if (cmake === null) return null;
  return {
    name,
    version,
    dir,
    targetName: expoModuleTargetName(name),
    codegen: null,
    declaredTizenCMake: cmake,
  };
}

function declaredCMake(
  visit: Visit,
  manifest: LibraryPackage,
  name: string,
  warnings: string[],
): string | null {
  if (manifest.tizen === undefined) return tizenExpoModuleCMake(visit.dir, name, warnings);
  const declaration = tizenDeclarationSchema.safeParse(manifest.tizen);
  if (declaration.success) return join(visit.dir, declaration.data.cmake);
  warnings.push(`${name}: its "tizen" field needs a "cmake" path; ignored`);
  return null;
}

function describeLibrary(
  visit: Visit,
  manifest: LibraryPackage,
  warnings: string[],
): NativeLibrary | null {
  const name = manifest.name ?? visit.fallbackName;
  const version = manifest.version ?? '';
  if (manifest.codegenConfig === undefined) {
    return describeExpoModule(visit.dir, name, version, warnings);
  }
  const codegen = codegenConfigSchema.safeParse(manifest.codegenConfig);
  if (!codegen.success) {
    warnings.push(`${name}: its codegenConfig is not one React Native's codegen reads; skipped`);
    return null;
  }
  return {
    name,
    version,
    dir: visit.dir,
    targetName: codegen.data.name,
    codegen: codegen.data,
    declaredTizenCMake: declaredCMake(visit, manifest, name, warnings),
  };
}

function addLibrary(library: NativeLibrary, libraries: NativeLibrary[], warnings: string[]): void {
  const clash = libraries.find((found) => found.targetName === library.targetName);
  if (clash === undefined) {
    libraries.push(library);
    return;
  }
  warnings.push(`${library.name} at ${library.dir}: ${clash.name} at ${clash.dir} is used instead`);
}

function dependenciesToVisit(
  visit: Visit,
  manifest: LibraryPackage,
  visited: Set<string>,
): Visit[] {
  if (visit.depth >= MAX_DEPTH) return [];
  const next: Visit[] = [];
  for (const name of dependencyNames(manifest, visit.depth)) {
    const dir = resolveFrom(visit.dir, name);
    if (dir === null || visited.has(dir)) continue;
    visited.add(dir);
    next.push({ dir, depth: visit.depth + 1, fallbackName: name });
  }
  return next;
}

/**
 * Walks the app's dependency tree the way Expo's autolinking does (dependencies
 * and non-optional peer dependencies at every depth, devDependencies of the app
 * only) and returns every package with a `codegenConfig` or a Tizen Expo
 * module, the app included, except the packages at `excludedDirs` (the app's
 * react-native), then the app's local Expo modules that support Tizen. When two
 * share a target name, the one found first, nearest the app, wins.
 */
export function findNativeLibraries(appRoot: string, excludedDirs: readonly string[]): LibraryScan {
  const excluded = new Set(excludedDirs.map((dir) => realpathSync(dir)));
  const root = realpathSync(appRoot);
  const visited = new Set<string>([root]);
  const queue: Visit[] = [{ dir: root, depth: 0, fallbackName: basename(root) }];
  const libraries: NativeLibrary[] = [];
  const warnings: string[] = [];
  for (let visit = queue.shift(); visit !== undefined; visit = queue.shift()) {
    const manifest = readPackage(visit.dir);
    if (manifest === null) continue;
    const library = excluded.has(visit.dir) ? null : describeLibrary(visit, manifest, warnings);
    if (library !== null) addLibrary(library, libraries, warnings);
    queue.push(...dependenciesToVisit(visit, manifest, visited));
  }
  for (const local of localExpoModules(root)) {
    const library = describeExpoModule(local.dir, local.name, '', warnings);
    if (library !== null) addLibrary(library, libraries, warnings);
  }
  libraries.sort((left, right) => left.name.localeCompare(right.name));
  return { libraries, warnings };
}
