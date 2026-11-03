import { z } from 'zod';

/** The privileges Samsung's voice service asks of an app it talks to. */
export const VOICE_PRIVILEGES = [
  'http://developer.samsung.com/privilege/voicecontrol',
  'http://developer.samsung.com/privilege/microphone',
] as const;

/** What a personal Smart Hub preview needs: the service's fetch, and starting it. */
export const PERSONAL_PREVIEW_PRIVILEGES = [
  'http://tizen.org/privilege/internet',
  'http://tizen.org/privilege/appmanager.launch',
] as const;

/** Samsung takes one personal preview update per 10 minutes. */
export const MIN_PREVIEW_REFRESH_MINUTES = 10;

/** The placeholders a voice `searchUrl` may hold, each filled URI-encoded. */
export const SEARCH_URL_PLACEHOLDERS = [
  'query',
  'utterance',
  'title',
  'genre',
  'cast',
  'contentType',
] as const;

const PLACEHOLDER = /\{([^{}]*)\}/g;
const PLACEHOLDER_LIST = SEARCH_URL_PLACEHOLDERS.map((name) => '{' + name + '}').join(', ');

const searchUrlSchema = z
  .string()
  .regex(/^[a-z][a-z0-9+.-]*:/i, 'expected a URL with a scheme, such as myapp://search?q={query}')
  .refine((url) => [...url.matchAll(PLACEHOLDER)].length > 0, {
    message: `expected at least one placeholder: ${PLACEHOLDER_LIST}`,
  })
  .refine(
    (url) =>
      [...url.matchAll(PLACEHOLDER)].every(([, name]) =>
        (SEARCH_URL_PLACEHOLDERS as readonly string[]).includes(name ?? ''),
      ),
    { message: `unknown placeholder; known: ${SEARCH_URL_PLACEHOLDERS.join(', ')}` },
  );

/** `expo.tizen.voice`. */
export const voiceConfigSchema = z.union([
  z.literal(true),
  z.object({ searchUrl: searchUrlSchema.optional() }).strict(),
]);

/** `expo.tizen.resolution`. */
export const resolutionSchema = z.enum(['1080p', '4k']);

/** `expo.tizen.preview`. */
export const previewConfigSchema = z.union([
  z.object({ endpoint: z.url({ protocol: /^https?$/ }) }).strict(),
  z
    .object({
      refreshMinutes: z
        .number()
        .int()
        .min(MIN_PREVIEW_REFRESH_MINUTES)
        .max(7 * 24 * 60),
    })
    .strict(),
]);

/** Samsung's voice interaction, on. */
export interface VoiceConfig {
  /** Opened through `Linking` for a voice search no listener takes; null opens nothing. */
  readonly searchUrl: string | null;
}

/** The Smart Hub preview: tiles from a URL Smart Hub fetches, or tiles the app publishes. */
export type PreviewConfig =
  | { readonly kind: 'public'; readonly endpoint: string }
  | { readonly kind: 'personal'; readonly refreshMinutes: number };

/** The UI plane the app asks the window manager for. */
export type Resolution = z.infer<typeof resolutionSchema>;

/** The TV integrations an app opted into. */
export interface TvFeatures {
  readonly voice: VoiceConfig | null;
  readonly preview: PreviewConfig | null;
  readonly resolution: Resolution;
  /** expo-video's config plugin asked for `supportsBackgroundPlayback`. */
  readonly backgroundMedia: boolean;
  /** expo-video's config plugin asked for `supportsPictureInPicture`, which Tizen cannot do. */
  readonly pictureInPicture: boolean;
}

const pluginEntrySchema = z.union([z.string(), z.tuple([z.string()]).rest(z.unknown())]);

const expoVideoOptionsSchema = z
  .object({
    supportsBackgroundPlayback: z.boolean().optional(),
    supportsPictureInPicture: z.boolean().optional(),
  })
  .loose();

function expoVideoOptions(plugins: unknown): z.infer<typeof expoVideoOptionsSchema> {
  if (!Array.isArray(plugins)) return {};
  for (const entry of plugins) {
    const parsed = pluginEntrySchema.safeParse(entry);
    if (!parsed.success || typeof parsed.data === 'string') continue;
    const [name, options] = parsed.data;
    if (name !== 'expo-video') continue;
    const checked = expoVideoOptionsSchema.safeParse(options ?? {});
    return checked.success ? checked.data : {};
  }
  return {};
}

function voiceConfig(voice: z.infer<typeof voiceConfigSchema> | undefined): VoiceConfig | null {
  if (voice === undefined) return null;
  return { searchUrl: voice === true ? null : (voice.searchUrl ?? null) };
}

function previewConfig(
  preview: z.infer<typeof previewConfigSchema> | undefined,
): PreviewConfig | null {
  if (preview === undefined) return null;
  if ('endpoint' in preview) return { kind: 'public', endpoint: preview.endpoint };
  return { kind: 'personal', refreshMinutes: preview.refreshMinutes };
}

/** The TV features `expo.tizen` and expo-video's config plugin options turn on. */
export function resolveTvFeatures(
  tizen: {
    readonly voice?: z.infer<typeof voiceConfigSchema>;
    readonly preview?: z.infer<typeof previewConfigSchema>;
    readonly resolution?: Resolution;
  },
  plugins: unknown,
): TvFeatures {
  const video = expoVideoOptions(plugins);
  return {
    voice: voiceConfig(tizen.voice),
    preview: previewConfig(tizen.preview),
    resolution: tizen.resolution ?? '1080p',
    backgroundMedia: video.supportsBackgroundPlayback === true,
    pictureInPicture: video.supportsPictureInPicture === true,
  };
}

/** The privileges the features need on top of the app's own. */
export function featurePrivileges(features: TvFeatures): readonly string[] {
  return [
    ...(features.voice === null ? [] : VOICE_PRIVILEGES),
    ...(features.preview?.kind === 'personal' ? PERSONAL_PREVIEW_PRIVILEGES : []),
  ];
}
