import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { loadAppProject } from '../app/app-project.ts';
import { checking, failed, passed, type CheckResult } from './check-result.ts';

const CONFIG_FILES = ['app.json', 'app.config.js', 'app.config.ts'];

/** The floor the app in `dir` sets with the plugin's `platform` prop, or undefined when it sets none or cannot be read. */
export function appFloor(dir: string): string | undefined {
  if (!CONFIG_FILES.some((name) => existsSync(join(dir, name)))) return undefined;
  try {
    return loadAppProject(dir).config.platform;
  } catch {
    return undefined;
  }
}

/** The app in `dir`, when there is one: its Tizen config and its dependencies. */
export function checkApp(dir: string): Promise<CheckResult> {
  if (!CONFIG_FILES.some((name) => existsSync(join(dir, name)))) {
    return Promise.resolve({
      title: 'App',
      status: 'skip',
      detail: 'no Expo app here (pass --project)',
    });
  }
  return checking('App', () => {
    const app = loadAppProject(dir);
    if (app.config.deprecation !== null) {
      return failed(
        'App',
        `${app.config.packageId} ${app.config.version}`,
        app.config.deprecation,
        'warn',
      );
    }
    return passed(
      'App',
      `${app.config.packageId} ${app.config.version}, ${app.packages.reactNative.name} ${app.packages.reactNative.version}`,
    );
  });
}
