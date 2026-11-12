import { z } from 'zod';
import type { WebOSAppConfig } from '../config/webos-config.ts';

/** The file the native host reads at startup, from the package's `res` directory. */
export const RUNTIME_CONFIG_FILE = 'react-native-webos.json';

/** The port a development build streams its logs to, unless told otherwise. */
export const DEFAULT_LOG_PORT = 8099;

/** Where a development build streams its logs. */
export interface LogEndpoint {
  readonly host: string;
  readonly port: number;
}

const runtimeConfigSchema = z.object({
  appKey: z.string().min(1),
  packageId: z.string().min(1),
  bundleFile: z.string().min(1),
  bundleUrl: z.url().optional(),
  development: z.boolean(),
  log: z.object({ host: z.string().min(1), port: z.number().int().min(1).max(65535) }).optional(),
});

type RuntimeConfig = z.infer<typeof runtimeConfigSchema>;

/**
 * The runtime config of a package. A development build streams its logs and
 * shows fatal JavaScript errors on screen; a release one does neither. With a
 * `bundleUrl`, the runtime loads the bundle from that dev server.
 */
export function runtimeConfig(
  bundleFile: string,
  log: LogEndpoint | null,
  config: Pick<WebOSAppConfig, 'packageId'>,
  bundleUrl?: string,
): RuntimeConfig {
  return runtimeConfigSchema.parse({
    appKey: 'main',
    packageId: config.packageId,
    bundleFile,
    ...(bundleUrl === undefined ? {} : { bundleUrl }),
    development: log !== null,
    ...(log === null ? {} : { log: { host: log.host, port: log.port } }),
  });
}
