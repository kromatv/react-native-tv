import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';

/** An app in a temporary directory, for tests that read one from disk. */
export interface TestApp {
  readonly root: string;
  write(file: string, content: unknown): void;
  dependOn(...names: string[]): void;
  platformPackage(name: string, declared: Record<string, unknown>, metroEntry?: string): void;
  remove(): void;
}

export function addsPlatform(platform: string): string {
  return `exports.withTvPlatform = (config) => ({ ...config, platforms: [...(config.platforms ?? []), '${platform}'] });`;
}

export function testApp(): TestApp {
  const root = mkdtempSync(join(tmpdir(), 'rn-tv-app-'));
  const write = (file: string, content: unknown) => {
    const path = join(root, file);
    mkdirSync(dirname(path), { recursive: true });
    writeFileSync(path, typeof content === 'string' ? content : JSON.stringify(content));
  };
  return {
    root,
    write,
    dependOn: (...names) =>
      write('package.json', {
        dependencies: Object.fromEntries(names.map((name) => [name, '1.0.0'])),
      }),
    platformPackage: (name, declared, metroEntry = '') => {
      write(`node_modules/${name}/package.json`, {
        name,
        bin: { [`rn-${name}`]: 'cli.js' },
        exports: { './metro': './metro.cjs', './package.json': './package.json' },
        reactNativeTvPlatform: { metro: './metro', ...declared },
      });
      write(`node_modules/${name}/metro.cjs`, metroEntry);
    },
    remove: () => rmSync(root, { recursive: true, force: true }),
  };
}
