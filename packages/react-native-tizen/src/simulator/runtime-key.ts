import { createHash } from 'node:crypto';
import type { ResolvedPackage } from '../app/app-packages.ts';

/** A native library as a simulator runtime compiles it in. */
export interface KeyedLibrary {
  readonly name: string;
  readonly version: string;
  readonly dir: string;
  readonly implementation: string | null;
}

/** The native code a simulator runtime build compiles besides this package's runtime/. */
export interface RuntimeCode {
  readonly packages: readonly ResolvedPackage[];
  readonly libraries: readonly KeyedLibrary[];
}

function compareNames(left: string, right: string): number {
  if (left === right) return 0;
  return left < right ? -1 : 1;
}

/** What a runtime key looks like; the guest scripts check the same. */
export const RUNTIME_KEY = /^[0-9a-f]{12}$/;

/**
 * The name of the simulator runtime build for `code`: apps that compile the
 * same native code in share one build, and an app with other libraries keeps
 * its own. Paths must be real paths, so two spellings of one directory agree.
 */
export function simulatorRuntimeKey(code: RuntimeCode): string {
  const packages = code.packages.map((pkg) => [pkg.name, pkg.version, pkg.dir]);
  const libraries = [...code.libraries]
    .sort((left, right) => compareNames(left.name, right.name))
    .map((library) => [library.name, library.version, library.dir, library.implementation]);
  const canonical = JSON.stringify({ packages, libraries });
  return createHash('sha256').update(canonical).digest('hex').slice(0, 12);
}
