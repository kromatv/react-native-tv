import { existsSync } from 'node:fs';
import { createRequire } from 'node:module';
import { join, resolve } from 'node:path';
import { z } from 'zod';
import { resolveTizenConfig, type TizenAppConfig } from '../config/tizen-config.ts';
import { isTizenPluginEntry } from '../config/plugin-entry.ts';
import { FixableError } from '../errors.ts';
import { resolveAppPackages, type AppPackages } from './app-packages.ts';
import { readExpoConfig } from './expo-config.ts';

/** An Expo app, read once and shared by every step that builds it. */
export interface AppProject {
  readonly root: string;
  readonly config: TizenAppConfig;
  readonly entryFile: string;
  readonly packages: AppPackages;
  readonly tizenDir: string;
}

type ResolveEntryPoint = (root: string, options: { platform: string }) => unknown;

const isFunction = (value: unknown) => typeof value === 'function';
const pathsModuleSchema = z.object({ resolveEntryPoint: z.custom<ResolveEntryPoint>(isFunction) });

function readAppConfig(root: string): unknown {
  try {
    return readExpoConfig(root);
  } catch (error) {
    const reason = error instanceof Error ? error.message : String(error);
    throw new FixableError(
      `Could not read the app config: ${reason}`,
      'Fix app.json (or app.config.js).',
    );
  }
}

function resolveEntryFile(root: string, requireFromApp: NodeJS.Require): string {
  const { resolveEntryPoint } = pathsModuleSchema.parse(requireFromApp('expo/config/paths'));
  const entry = z
    .string()
    .nullable()
    .parse(resolveEntryPoint(root, { platform: 'tizen' }));
  if (entry !== null) return entry;
  throw new FixableError('The app has no entry file', 'Set "main" in package.json.');
}

function checkEntryFile(entry: string): string {
  if (existsSync(entry)) return entry;
  throw new FixableError(
    `The entry file ${entry} does not exist`,
    'Check the path given to --entry.',
  );
}

/**
 * Reads the Expo app at `root`: its config, entry file and native packages.
 * `entry`, an absolute path, replaces the entry Expo resolves from the app.
 */
export function loadAppProject(root: string, entry?: string): AppProject {
  const projectRoot = resolve(root);
  if (!existsSync(join(projectRoot, 'package.json'))) {
    throw new FixableError(
      `${projectRoot} is not an app: it has no package.json`,
      "Run rn-tizen in your app's directory, or pass --project <dir>.",
    );
  }
  const packages = resolveAppPackages(projectRoot);
  const requireFromApp = createRequire(join(projectRoot, 'package.json'));
  return {
    root: projectRoot,
    config: resolveTizenConfig(readAppConfig(projectRoot), (name) =>
      isTizenPluginEntry(projectRoot, name),
    ),
    entryFile:
      entry === undefined ? resolveEntryFile(projectRoot, requireFromApp) : checkEntryFile(entry),
    packages,
    tizenDir: join(projectRoot, 'tizen'),
  };
}
