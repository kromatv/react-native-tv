import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';

/** An app in a temporary directory, for tests that read one from disk. */
export interface TestApp {
  readonly root: string;
  write(file: string, content: unknown): void;
  dependOn(...names: string[]): void;
  platformPackage(name: string, declared: Record<string, unknown>, metroEntry?: string): void;
  /** Installs react-native-tv with `names` as its dependencies, and each as a platform only it reaches. */
  bring(
    names: Record<string, Record<string, unknown>>,
    metroEntry?: (name: string) => string,
  ): void;
  remove(): void;
}

export function addsPlatform(platform: string): string {
  return `exports.withTvPlatform = (config) => ({ ...config, platforms: [...(config.platforms ?? []), '${platform}'] });`;
}

/** Where a test app has react-native-tv, and so where what only that package reaches is installed. */
export const TV_PACKAGE_DIR = 'node_modules/@kromatv/react-native-tv';

export function testApp(): TestApp {
  const root = mkdtempSync(join(tmpdir(), 'rn-tv-app-'));
  const write = (file: string, content: unknown) => {
    const path = join(root, file);
    mkdirSync(dirname(path), { recursive: true });
    writeFileSync(path, typeof content === 'string' ? content : JSON.stringify(content));
  };
  const platformPackage = (
    dir: string,
    name: string,
    declared: Record<string, unknown>,
    metroEntry: string,
  ) => {
    write(`${dir}/${name}/package.json`, {
      name,
      bin: { [`rn-${name}`]: 'cli.js' },
      exports: {
        './metro': './metro.cjs',
        './app.plugin.js': './app.plugin.js',
        './package.json': './package.json',
      },
      reactNativeTvPlatform: { metro: './metro', ...declared },
    });
    write(`${dir}/${name}/metro.cjs`, metroEntry);
    write(`${dir}/${name}/app.plugin.js`, '');
  };
  return {
    root,
    write,
    dependOn: (...names) =>
      write('package.json', {
        dependencies: Object.fromEntries(names.map((name) => [name, '1.0.0'])),
      }),
    platformPackage: (name, declared, metroEntry = '') =>
      platformPackage('node_modules', name, declared, metroEntry),
    bring: (names, metroEntry = () => '') => {
      write(`${TV_PACKAGE_DIR}/package.json`, {
        name: '@kromatv/react-native-tv',
        dependencies: Object.fromEntries(Object.keys(names).map((name) => [name, '1.0.0'])),
      });
      for (const [name, declared] of Object.entries(names)) {
        platformPackage(`${TV_PACKAGE_DIR}/node_modules`, name, declared, metroEntry(name));
      }
    },
    remove: () => rmSync(root, { recursive: true, force: true }),
  };
}
