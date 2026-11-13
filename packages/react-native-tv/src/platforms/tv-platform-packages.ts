import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, isAbsolute, join, relative, resolve } from 'node:path';
import { z } from 'zod';

/** The `package.json` key a TV platform package describes itself under. */
export const TV_PLATFORM_KEY = 'reactNativeTvPlatform';

/** This package, whose own dependencies are the platforms it brings to an app. */
export const TV_PACKAGE = '@kromatv/react-native-tv';

/** A TV platform the app has: one of its dependencies, or one this package brings. */
export interface TvPlatformPackage {
  readonly name: string;
  /** The module that holds its Metro wrapper, as `from` requires it. */
  readonly metro: string;
  /** The `package.json` it resolves from: the app's, or this package's when the app cannot reach it. */
  readonly from: string;
  /** The platform it adds to Metro, such as `webos`, when it names one. */
  readonly platform: string | null;
  /** The script of the command it names, when its `bin` has that command. */
  readonly cli: string | null;
}

const MAX_MANIFEST_BYTES = 1024 * 1024;
const DEPENDENCY_FIELDS = [
  'dependencies',
  'devDependencies',
  'optionalDependencies',
  'peerDependencies',
] as const;

const dependencyNames = z.record(z.string(), z.unknown()).optional();
const appManifestSchema = z.looseObject({
  dependencies: dependencyNames,
  devDependencies: dependencyNames,
  optionalDependencies: dependencyNames,
  peerDependencies: dependencyNames,
});
const ownManifestSchema = z.looseObject({ dependencies: dependencyNames });
const platformManifestSchema = z.looseObject({
  [TV_PLATFORM_KEY]: z.object({
    metro: z.string().startsWith('./').max(256),
    platform: z
      .string()
      .regex(/^[a-z][a-z0-9]{0,31}$/)
      .optional(),
    cli: z.string().max(64).optional(),
  }),
  bin: z.record(z.string(), z.string().max(256)).optional().catch(undefined),
});

function readManifest(file: string): unknown {
  try {
    const text = readFileSync(file, 'utf8');
    return text.length > MAX_MANIFEST_BYTES ? null : JSON.parse(text);
  } catch {
    return null;
  }
}

function scriptInside(dir: string, script: string | undefined): string | null {
  if (script === undefined) return null;
  const file = resolve(dir, script);
  const fromDir = relative(dir, file);
  return fromDir.startsWith('..') || isAbsolute(fromDir) ? null : file;
}

/** The first of `manifests` that `specifier` resolves from, with the file it resolves to. */
export function resolvedFrom(
  manifests: readonly string[],
  specifier: string,
): { readonly from: string; readonly file: string } | null {
  for (const from of manifests) {
    try {
      return { from, file: createRequire(from).resolve(specifier) };
    } catch {
      continue;
    }
  }
  return null;
}

/**
 * The `package.json` files the TV packages of the app at `projectRoot` resolve
 * from, in order: the app's, then this package's as the app has it.
 */
export function tvManifests(projectRoot: string): string[] {
  const appManifest = join(projectRoot, 'package.json');
  const own = resolvedFrom([appManifest], `${TV_PACKAGE}/package.json`);
  return own === null ? [appManifest] : [appManifest, own.file];
}

function platformPackage(manifests: readonly string[], name: string): TvPlatformPackage | null {
  const found = resolvedFrom(manifests, `${name}/package.json`);
  if (found === null) return null;
  const parsed = platformManifestSchema.safeParse(readManifest(found.file));
  if (!parsed.success) return null;
  const declared = parsed.data[TV_PLATFORM_KEY];
  const script = declared.cli === undefined ? undefined : parsed.data.bin?.[declared.cli];
  return {
    name,
    metro: `${name}/${declared.metro.slice(2)}`,
    from: found.from,
    platform: declared.platform ?? null,
    cli: scriptInside(dirname(found.file), script),
  };
}

/**
 * The TV platforms of the app at `projectRoot`, by name: every dependency of
 * the app, and of this package as the app has it, whose `package.json` has a
 * `reactNativeTvPlatform` with the subpath of its Metro entry, as in
 * `{ "metro": "./metro", "platform": "webos", "cli": "rn-webos" }`. The copy
 * the app resolves wins over the one this package brings. A package that keeps
 * its `package.json` out of its `exports` is not found.
 */
export function tvPlatformPackages(projectRoot: string): TvPlatformPackage[] {
  const manifests = tvManifests(projectRoot);
  const [app, own] = manifests.map((manifest) => readManifest(manifest));
  const appDependencies = appManifestSchema.safeParse(app);
  if (!appDependencies.success) return [];
  const brought = ownManifestSchema.safeParse(own);
  const names = new Set([
    ...DEPENDENCY_FIELDS.flatMap((field) => Object.keys(appDependencies.data[field] ?? {})),
    ...Object.keys((brought.success ? brought.data.dependencies : undefined) ?? {}),
  ]);
  return [...names]
    .sort((first, second) => first.localeCompare(second))
    .flatMap((name) => platformPackage(manifests, name) ?? []);
}
