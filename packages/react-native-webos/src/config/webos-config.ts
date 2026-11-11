import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { DEFAULT_INTL_LOCALES } from '../metro/intl-locale-data.ts';
import { appIdSchema, appVersionSchema } from './app-id.ts';
import {
  MISSING_SETTINGS,
  PLUGIN_WHERE,
  WEBOS_PLUGIN,
  webosPluginProps,
  type PluginMatcher,
} from './webos-settings.ts';

const CONFIG_HINT = 'Fix app.json. Every field is described in the CLI docs, under Configuration.';

/** LG's limit on `title` in `appinfo.json`. */
export const MAX_TITLE_LENGTH = 20;

/**
 * The ACG groups of the Luna calls the runtime itself makes, the five LG's
 * Flutter tool always declares. webOS TV 26 Re:New refuses a call outside the
 * groups an app declares.
 */
export const RUNTIME_PERMISSIONS = [
  'lunabus.query',
  'application.launcher',
  'settings.query',
  'systemconfig.query',
  'tts.operation',
] as const;

const titleSchema = z
  .string()
  .min(1)
  .max(MAX_TITLE_LENGTH, `expected ${MAX_TITLE_LENGTH} characters or fewer`);

const localeSchema = z
  .string()
  .regex(/^[a-z]{2,3}(-[A-Za-z0-9]{2,8})*$/, 'expected a BCP 47 language tag such as fr or pt-BR');

const pngPathSchema = z.string().regex(/\.png$/i, 'expected the path of a PNG');

/** The webOS settings: the config plugin's props. */
export const webosSettingsSchema = z
  .object({
    package: appIdSchema,
    title: titleSchema.optional(),
    version: appVersionSchema.optional(),
    vendor: z.string().min(1).optional(),
    icon: pngPathSchema.optional(),
    iconColor: z.string().min(1).optional(),
    splash: pngPathSchema.optional(),
    permissions: z.array(z.string().min(1)).optional(),
    locales: z.array(localeSchema).min(1).optional(),
  })
  .strict();

const expoConfigSchema = z.object({
  name: z.string().min(1),
  version: z.string().optional(),
  icon: z.string().optional(),
  plugins: z.unknown().optional(),
});

/** Everything a webOS package is built from, defaults applied. */
export interface WebOSAppConfig {
  /** The app id: `id` in `appinfo.json`, and the package's name. */
  readonly packageId: string;
  readonly title: string;
  readonly version: string;
  readonly vendor: string | undefined;
  /** A PNG the two launcher icons are scaled from, relative to the app. */
  readonly icon: string | undefined;
  readonly iconColor: string | undefined;
  /** A 1920x1080 PNG shown while the app loads, relative to the app. */
  readonly splash: string | undefined;
  /** The ACG groups `appinfo.json` declares in `requiredACG`. */
  readonly permissions: readonly string[];
  /** The locales `Intl` formats in, besides the root locale ICU always keeps. */
  readonly locales: readonly string[];
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

/** Every problem with a set of webOS settings, one per line, each named from `where`; null when there is none. */
export function webosSettingsIssues(settings: unknown, where: string): string | null {
  const parsed = webosSettingsSchema.safeParse(settings);
  return parsed.success ? null : formatIssues(parsed.error, where);
}

function checked<Value>(schema: z.ZodType<Value>, value: Value, field: string, prop: string) {
  const parsed = schema.safeParse(value);
  if (parsed.success) return parsed.data;
  const reason = parsed.error.issues[0]?.message ?? 'not valid';
  throw configError(`${field}: ${String(value)}: ${reason}; set ${PLUGIN_WHERE}.${prop}`);
}

/**
 * Validates an Expo config and resolves its webOS config, the config plugin's
 * props, against the Expo-wide fields. A {@link FixableError} names every bad
 * field.
 */
export function resolveWebOSConfig(
  expo: unknown,
  isWebOSPlugin: PluginMatcher = (name) => name === WEBOS_PLUGIN,
): WebOSAppConfig {
  const base = expoConfigSchema.safeParse(expo);
  if (!base.success) throw configError(formatIssues(base.error, 'expo'));
  const props = webosPluginProps(base.data.plugins, isWebOSPlugin);
  if (props === undefined) throw configError(MISSING_SETTINGS);
  const webos = webosSettingsSchema.safeParse(props);
  if (!webos.success) throw configError(formatIssues(webos.error, PLUGIN_WHERE));

  const icon = webos.data.icon ?? base.data.icon;
  return {
    packageId: webos.data.package,
    title: webos.data.title ?? checked(titleSchema, base.data.name, 'expo.name', 'title'),
    version:
      webos.data.version ??
      checked(appVersionSchema, base.data.version ?? '1.0.0', 'expo.version', 'version'),
    vendor: webos.data.vendor,
    icon: icon === undefined ? undefined : checked(pngPathSchema, icon, 'expo.icon', 'icon'),
    iconColor: webos.data.iconColor,
    splash: webos.data.splash,
    permissions: [...new Set([...RUNTIME_PERMISSIONS, ...(webos.data.permissions ?? [])])],
    locales: [...new Set(webos.data.locales ?? DEFAULT_INTL_LOCALES)],
  };
}
