import { z } from 'zod';

/** One module of a platform's API, as an app imports it from this package. */
export interface PlatformApiModule {
  /** Its path under this package, without extension: `tizen/preview`, or `tizen` for the module named after the platform. */
  readonly file: string;
  /** What it re-exports, such as `@kromatv/react-native-tizen/preview`. */
  readonly specifier: string;
}

const manifestSchema = z.looseObject({
  name: z.string().min(1).max(214),
  exports: z.record(z.string(), z.unknown()),
  reactNativeTvPlatform: z.object({
    metro: z.string(),
    platform: z.string().regex(/^[a-z][a-z0-9]{0,31}$/),
  }),
});

const API_SUBPATH = /^\.\/([a-z0-9][a-z0-9-]{0,63})$/;

/**
 * The API modules of the platform package with this `package.json`: every
 * subpath of its `exports` that is one plain name, but its Metro entry. None
 * for a package that names no platform.
 */
export function platformApiModules(manifest: unknown): PlatformApiModule[] {
  const parsed = manifestSchema.safeParse(manifest);
  if (!parsed.success) return [];
  const { name, exports, reactNativeTvPlatform: declared } = parsed.data;
  return Object.keys(exports)
    .filter((subpath) => subpath !== declared.metro)
    .flatMap((subpath) => API_SUBPATH.exec(subpath)?.[1] ?? [])
    .sort((first, second) => first.localeCompare(second))
    .map((module) => ({
      file: module === declared.platform ? module : `${declared.platform}/${module}`,
      specifier: `${name}/${module}`,
    }));
}
