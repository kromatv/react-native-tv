import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, isAbsolute, join, relative, resolve } from 'node:path';
import { z } from 'zod';

/** The `package.json` key a TV platform package describes itself under. */
export const TV_PLATFORM_KEY = 'reactNativeTvPlatform';

/** A dependency of the app that is a TV platform. */
export interface TvPlatformPackage {
  readonly name: string;
  /** The module that holds its Metro wrapper, as the app requires it. */
  readonly metro: string;
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

function platformPackage(fromApp: NodeJS.Require, name: string): TvPlatformPackage | null {
  let manifest: string;
  try {
    manifest = fromApp.resolve(`${name}/package.json`);
  } catch {
    return null;
  }
  const parsed = platformManifestSchema.safeParse(readManifest(manifest));
  if (!parsed.success) return null;
  const declared = parsed.data[TV_PLATFORM_KEY];
  const script = declared.cli === undefined ? undefined : parsed.data.bin?.[declared.cli];
  return {
    name,
    metro: `${name}/${declared.metro.slice(2)}`,
    platform: declared.platform ?? null,
    cli: scriptInside(dirname(manifest), script),
  };
}

/**
 * The TV platforms among the dependencies of the app at `projectRoot`, by
 * name: every package whose `package.json` has a `reactNativeTvPlatform` with
 * the subpath of its Metro entry, as in
 * `{ "metro": "./metro", "platform": "webos", "cli": "rn-webos" }`. A package
 * that keeps its `package.json` out of its `exports` is not found.
 */
export function tvPlatformPackages(projectRoot: string): TvPlatformPackage[] {
  const app = appManifestSchema.safeParse(readManifest(join(projectRoot, 'package.json')));
  if (!app.success) return [];
  const fromApp = createRequire(join(projectRoot, 'package.json'));
  const names = new Set(DEPENDENCY_FIELDS.flatMap((field) => Object.keys(app.data[field] ?? {})));
  return [...names]
    .sort((first, second) => first.localeCompare(second))
    .flatMap((name) => platformPackage(fromApp, name) ?? []);
}
