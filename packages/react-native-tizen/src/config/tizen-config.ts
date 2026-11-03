import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { DEFAULT_INTL_LOCALES } from '../metro/intl-locale-data.ts';
import { platformVersionSchema } from '../tizen/target.ts';
import { findTizenSettings, TIZEN_PLUGIN, type PluginMatcher } from './tizen-settings.ts';
import {
  featurePrivileges,
  previewConfigSchema,
  resolutionSchema,
  resolveTvFeatures,
  voiceConfigSchema,
  type TvFeatures,
} from './tv-features.ts';

const PRIVILEGE_PREFIX = 'http://tizen.org/privilege/';
const DEFAULT_PRIVILEGES = [`${PRIVILEGE_PREFIX}internet`];
const CONFIG_HINT = 'Fix app.json. Every field is described in the CLI docs, under Configuration.';

const packageIdSchema = z
  .string()
  .max(50)
  .regex(
    /^[A-Za-z]\w*(\.[A-Za-z]\w*)+$/,
    'expected a reverse-DNS id such as org.example.myapp, with letters, digits and underscores',
  );

const tizenVersionSchema = z
  .string()
  .regex(/^\d{1,3}\.\d{1,3}\.\d{1,5}$/, 'expected a version such as 1.0.0');

const localeSchema = z
  .string()
  .regex(/^[a-z]{2,3}(-[A-Za-z0-9]{2,8})*$/, 'expected a BCP 47 language tag such as fr or pt-BR');

const privilegeSchema = z
  .string()
  .min(1)
  .transform((name) => (name.includes('://') ? name : `${PRIVILEGE_PREFIX}${name}`));

/** The Tizen settings: the config plugin's props, or `expo.tizen` in earlier releases. */
export const tizenSettingsSchema = z
  .object({
    package: packageIdSchema,
    label: z.string().min(1).max(100).optional(),
    version: tizenVersionSchema.optional(),
    profile: z.string().min(1).optional(),
    privileges: z.array(privilegeSchema).optional(),
    platform: platformVersionSchema.optional(),
    icon: z.string().min(1).optional(),
    locales: z.array(localeSchema).min(1).optional(),
    voice: voiceConfigSchema.optional(),
    preview: previewConfigSchema.optional(),
    resolution: resolutionSchema.optional(),
  })
  .strict();

const expoConfigSchema = z.object({
  name: z.string().min(1),
  version: z.string().optional(),
  icon: z.string().optional(),
  plugins: z.unknown().optional(),
  tizen: z.unknown().optional(),
});

/** Everything a Tizen package is built from, defaults applied. */
export interface TizenAppConfig {
  readonly packageId: string;
  readonly label: string;
  readonly version: string;
  readonly profile: string | undefined;
  readonly privileges: readonly string[];
  readonly platform: string | undefined;
  readonly icon: string | undefined;
  /** The locales `Intl` formats in, besides the root locale ICU always keeps. */
  readonly locales: readonly string[];
  /** Voice, the Smart Hub preview, the UI resolution and background audio. */
  readonly features: TvFeatures;
  /** What to warn the user about the way the settings are written, if anything. */
  readonly deprecation: string | null;
}

function formatIssues(error: z.ZodError, root: string): string {
  return error.issues
    .map((issue) => {
      const path = [root, ...issue.path.map(String)].join('.');
      return `${path}: ${issue.message}`;
    })
    .join('\n');
}

function configError(message: string): FixableError {
  return new FixableError(message, CONFIG_HINT);
}

/** Every problem with a set of Tizen settings, one per line, each named from `where`; null when there is none. */
export function tizenSettingsIssues(settings: unknown, where: string): string | null {
  const parsed = tizenSettingsSchema.safeParse(settings);
  return parsed.success ? null : formatIssues(parsed.error, where);
}

/**
 * Validates an Expo config and resolves its Tizen config against the Expo-wide
 * fields: the config plugin's props, else `expo.tizen`. A {@link FixableError}
 * names every bad field.
 */
export function resolveTizenConfig(
  expo: unknown,
  isTizenPlugin: PluginMatcher = (name) => name === TIZEN_PLUGIN,
): TizenAppConfig {
  const base = expoConfigSchema.safeParse(expo);
  if (!base.success) throw configError(formatIssues(base.error, 'expo'));
  const found = findTizenSettings(base.data.plugins, base.data.tizen, isTizenPlugin);
  if ('error' in found) throw configError(found.error);
  const tizen = tizenSettingsSchema.safeParse(found.settings);
  if (!tizen.success) throw configError(formatIssues(tizen.error, found.where));

  const version = tizen.data.version ?? base.data.version ?? '1.0.0';
  const versionCheck = tizenVersionSchema.safeParse(version);
  if (!versionCheck.success) {
    throw configError(
      `expo.version: ${version} is not a Tizen version (x.y.z); set ${found.where}.version`,
    );
  }
  const features = resolveTvFeatures(tizen.data, base.data.plugins);
  return {
    packageId: tizen.data.package,
    label: tizen.data.label ?? base.data.name,
    version,
    profile: tizen.data.profile,
    privileges: [
      ...new Set([
        ...(tizen.data.privileges ?? DEFAULT_PRIVILEGES),
        ...featurePrivileges(features),
      ]),
    ],
    platform: tizen.data.platform,
    icon: tizen.data.icon ?? base.data.icon,
    locales: [...new Set(tizen.data.locales ?? DEFAULT_INTL_LOCALES)],
    features,
    deprecation: found.deprecation,
  };
}
