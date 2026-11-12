import { join } from 'node:path';
import { resolveTizenConfig } from '../config/tizen-config.ts';
import type { AppProject } from './app-project.ts';

/** An {@link AppProject} at `root` whose packages all resolve to `root`, configured by `tizen`. */
export function fakeApp(root: string, tizen: Record<string, unknown> = {}): AppProject {
  const installed = { name: 'installed', version: '1.0.0', dir: root };
  return {
    root,
    config: resolveTizenConfig({ name: 'Demo', tizen: { package: 'org.example.demo', ...tizen } }),
    entryFile: join(root, 'index.ts'),
    packages: {
      reactNative: { ...installed, name: 'react-native', version: '0.88.0' },
      expo: installed,
      expoModulesCore: installed,
      expoModulesJsi: installed,
    },
    tizenDir: join(root, 'tizen'),
  };
}
