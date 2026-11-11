import { join } from 'node:path';
import { resolveWebOSConfig } from '../config/webos-config.ts';
import { WEBOS_PLUGIN } from '../config/webos-settings.ts';
import type { AppProject } from './app-project.ts';

/** An {@link AppProject} at `root` whose packages all resolve to `root`, configured by `webos`. */
export function fakeApp(root: string, webos: Record<string, unknown> = {}): AppProject {
  const installed = { name: 'installed', version: '1.0.0', dir: root };
  return {
    root,
    config: resolveWebOSConfig({
      name: 'Demo',
      plugins: [[WEBOS_PLUGIN, { package: 'org.example.demo', ...webos }]],
    }),
    entryFile: join(root, 'index.ts'),
    packages: {
      reactNative: { ...installed, name: 'react-native', version: '0.88.0' },
      expo: installed,
      expoModulesCore: installed,
      expoModulesJsi: installed,
    },
    webosDir: join(root, 'webos'),
  };
}
