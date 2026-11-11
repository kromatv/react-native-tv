import { tizenSettingsIssues } from './tizen-config.ts';
import { TIZEN_PLUGIN } from './tizen-settings.ts';

/**
 * The config plugin `app.plugin.js` exports. Its props are the app's Tizen
 * settings, checked here so `expo config` and `expo start` report a mistake;
 * it changes nothing in the config, since Expo generates no Tizen project.
 */
export default function withReactNativeTizen<Config>(config: Config, props?: unknown): Config {
  if (props === undefined) return config;
  const issues = tizenSettingsIssues(props, `plugins["${TIZEN_PLUGIN}"]`);
  if (issues !== null) throw new Error(`Invalid ${TIZEN_PLUGIN} props:\n${issues}`);
  return config;
}
