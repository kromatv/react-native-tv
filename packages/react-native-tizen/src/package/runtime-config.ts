import { z } from 'zod';
import type { TizenAppConfig } from '../config/tizen-config.ts';
import { hasPreviewService, previewServiceAppId } from './feature-manifest.ts';

/** The file the native host reads at startup, from the package's `res` directory. */
export const RUNTIME_CONFIG_FILE = 'react-native-tizen.json';

/** The port a development build streams its logs to, unless told otherwise. */
export const DEFAULT_LOG_PORT = 8099;

/** Where a development build streams its logs. */
export interface LogEndpoint {
  readonly host: string;
  readonly port: number;
}

const runtimeConfigSchema = z.object({
  appKey: z.string().min(1),
  bundleFile: z.string().min(1),
  bundleUrl: z.url().optional(),
  development: z.boolean(),
  log: z.object({ host: z.string().min(1), port: z.number().int().min(1).max(65535) }).optional(),
  resolution: z.enum(['1080p', '4k']),
  backgroundMedia: z.boolean(),
  voice: z.object({ searchUrl: z.string().min(1).optional() }).optional(),
  preview: z
    .object({
      kind: z.enum(['public', 'personal']),
      refreshMinutes: z.number().int().positive().optional(),
      service: z.string().min(1).optional(),
    })
    .optional(),
});

type RuntimeConfig = z.infer<typeof runtimeConfigSchema>;

function previewFields(config: Pick<TizenAppConfig, 'packageId' | 'features'>) {
  const { preview } = config.features;
  if (preview === null) return {};
  if (preview.kind === 'public') return { preview: { kind: preview.kind } };
  return {
    preview: {
      kind: preview.kind,
      refreshMinutes: preview.refreshMinutes,
      ...(hasPreviewService(config.features)
        ? { service: previewServiceAppId(config.packageId) }
        : {}),
    },
  };
}

/**
 * The runtime config of a package. A development build streams its logs and
 * shows fatal JavaScript errors on screen; a release one does neither. With a
 * `bundleUrl`, the runtime loads the bundle from that dev server. The app's TV
 * features tell the host and the shell what to start.
 */
export function runtimeConfig(
  bundleFile: string,
  log: LogEndpoint | null,
  config: Pick<TizenAppConfig, 'packageId' | 'features'>,
  bundleUrl?: string,
): RuntimeConfig {
  const { voice, resolution, backgroundMedia } = config.features;
  return runtimeConfigSchema.parse({
    appKey: 'main',
    bundleFile,
    ...(bundleUrl === undefined ? {} : { bundleUrl }),
    development: log !== null,
    ...(log === null ? {} : { log: { host: log.host, port: log.port } }),
    resolution,
    backgroundMedia,
    ...(voice === null
      ? {}
      : { voice: voice.searchUrl === null ? {} : { searchUrl: voice.searchUrl } }),
    ...previewFields(config),
  });
}
