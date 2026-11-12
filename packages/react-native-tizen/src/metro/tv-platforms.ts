import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { join, posix } from 'node:path';
import { z } from 'zod';
import type { MetroConfigLike } from './with-tizen.ts';

/** The `package.json` key a TV platform package names its Metro entry under. */
export const TV_PLATFORM_KEY = 'reactNativeTvPlatform';

/** What a TV platform's Metro entry exports for `withTV` to apply. */
export const TV_PLATFORM_EXPORT = 'withTvPlatform';

/** Teaches a Metro config one TV platform, and returns it. */
export type TvPlatformWrapper = <Config extends MetroConfigLike>(config: Config) => Config;

/** A dependency of the app that is a TV platform, and the module that holds its wrapper. */
export interface TvPlatformPackage {
  readonly name: string;
  readonly metro: string;
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
  [TV_PLATFORM_KEY]: z.object({ metro: z.string().startsWith('./').max(256) }),
});
const entrySchema = z.looseObject({
  [TV_PLATFORM_EXPORT]: z.custom<TvPlatformWrapper>((value) => typeof value === 'function'),
});

function readManifest(file: string): unknown {
  try {
    const text = readFileSync(file, 'utf8');
    return text.length > MAX_MANIFEST_BYTES ? null : JSON.parse(text);
  } catch {
    return null;
  }
}

function metroEntry(fromApp: NodeJS.Require, name: string): string | null {
  let manifest: string;
  try {
    manifest = fromApp.resolve(`${name}/package.json`);
  } catch {
    return null;
  }
  const parsed = platformManifestSchema.safeParse(readManifest(manifest));
  return parsed.success ? posix.join(name, parsed.data[TV_PLATFORM_KEY].metro) : null;
}

/**
 * The TV platforms among the dependencies of the app at `projectRoot`, by
 * name: every package whose `package.json` has a `reactNativeTvPlatform` with
 * the subpath of its Metro entry, as in `{ "metro": "./metro" }`. A package
 * that keeps its `package.json` out of its `exports` is not found.
 */
export function tvPlatformPackages(projectRoot: string): TvPlatformPackage[] {
  const app = appManifestSchema.safeParse(readManifest(join(projectRoot, 'package.json')));
  if (!app.success) return [];
  const fromApp = createRequire(join(projectRoot, 'package.json'));
  const names = new Set(DEPENDENCY_FIELDS.flatMap((field) => Object.keys(app.data[field] ?? {})));
  return [...names]
    .sort((first, second) => first.localeCompare(second))
    .flatMap((name) => {
      const metro = metroEntry(fromApp, name);
      return metro === null ? [] : [{ name, metro }];
    });
}

/** The wrapper `platform` exports, loaded as the app at `projectRoot` resolves it. */
export function loadTvPlatform(
  projectRoot: string,
  platform: TvPlatformPackage,
): TvPlatformWrapper {
  const entry = entrySchema.safeParse(
    createRequire(join(projectRoot, 'package.json'))(platform.metro),
  );
  if (!entry.success) {
    throw new Error(
      `${platform.name} declares a TV platform, but ${platform.metro} exports no ${TV_PLATFORM_EXPORT} function`,
    );
  }
  return entry.data[TV_PLATFORM_EXPORT];
}
