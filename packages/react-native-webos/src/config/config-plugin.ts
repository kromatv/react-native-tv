import { webosSettingsIssues } from './webos-config.ts';
import { WEBOS_PLUGIN } from './webos-settings.ts';

/**
 * The config plugin `app.plugin.js` exports. Its props are the app's webOS
 * settings, checked here so `expo config` and `expo start` report a mistake;
 * it changes nothing in the config, since Expo generates no webOS project.
 */
export default function withReactNativeWebOS<Config>(config: Config, props?: unknown): Config {
  if (props === undefined) return config;
  const issues = webosSettingsIssues(props, `plugins["${WEBOS_PLUGIN}"]`);
  if (issues !== null) throw new Error(`Invalid ${WEBOS_PLUGIN} props:\n${issues}`);
  return config;
}
