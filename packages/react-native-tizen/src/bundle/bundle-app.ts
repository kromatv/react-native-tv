import { existsSync, readdirSync } from 'node:fs';
import { mkdir, rm, writeFile } from 'node:fs/promises';
import { createRequire } from 'node:module';
import { join } from 'node:path';
import type { AppProject } from '../app/app-project.ts';
import { FixableError } from '../errors.ts';
import { localesWithoutIntlData } from '../metro/intl-locale-data.ts';
import { INTL_LOCALES_ENV } from '../metro/tizen-babel-transformer-env.ts';
import { TIZEN_PLATFORM } from '../metro/with-tizen.ts';
import type { Task } from '../task.ts';
import { locateHermesc } from './hermesc.ts';

/** The file the native host loads, inside a package's `res` directory. */
export const BYTECODE_FILE = 'index.tizen.hbc';

const JAVASCRIPT_FILE = 'index.tizen.js';
const BUNDLE_MARKER = '.rn-tizen-bundle';

/** What {@link bundleApp} wrote. */
export interface AppBundle {
  readonly javascript: string;
  readonly bytecode: string;
  readonly assetsDir: string;
  /** A dev server the runtime loads the bundle from, instead of `bytecode`. */
  readonly bundleUrl?: string;
}

function requireIntlData(locales: readonly string[]): void {
  const missing = localesWithoutIntlData(locales);
  if (missing.length === 0) return;
  throw new FixableError(
    `expo.tizen.locales: no Intl data for ${missing.join(', ')}`,
    'Name locales FormatJS has data for, such as fr or pt-PT.',
  );
}

function bundledBefore(outDir: string): boolean {
  return [BUNDLE_MARKER, JAVASCRIPT_FILE].some((name) => existsSync(join(outDir, name)));
}

async function claimBundleDir(outDir: string): Promise<void> {
  if (existsSync(outDir) && readdirSync(outDir).length > 0 && !bundledBefore(outDir)) {
    throw new FixableError(
      `${outDir} holds files rn-tizen did not write`,
      'Bundle into a new or empty directory: every bundle replaces the assets/ folder of the one it writes to.',
    );
  }
  await mkdir(outDir, { recursive: true });
  await writeFile(join(outDir, BUNDLE_MARKER), '');
}

/**
 * Bundles the app for Tizen with Expo's own `export:embed`, through the app's
 * Metro config, with Intl data for the app's locales, then compiles the bundle
 * to Hermes bytecode. Refuses an `outDir` that holds files of another origin.
 */
export async function bundleApp(task: Task, app: AppProject, outDir: string): Promise<AppBundle> {
  requireIntlData(app.config.locales);
  await claimBundleDir(outDir);
  const bundle: AppBundle = {
    javascript: join(outDir, JAVASCRIPT_FILE),
    bytecode: join(outDir, BYTECODE_FILE),
    assetsDir: join(outDir, 'assets'),
  };
  await rm(bundle.assetsDir, { recursive: true, force: true });
  await mkdir(bundle.assetsDir, { recursive: true });
  const expoCli = createRequire(join(app.packages.expo.dir, 'package.json')).resolve('./bin/cli');
  task.progress('Metro');
  await task.run(
    [
      'node',
      expoCli,
      'export:embed',
      '--platform',
      TIZEN_PLATFORM,
      '--dev',
      'false',
      '--entry-file',
      app.entryFile,
      '--bundle-output',
      bundle.javascript,
      // Metro writes each asset at its httpServerLocation, which already starts with `assets/`.
      '--assets-dest',
      outDir,
    ],
    { cwd: app.root, env: { ...process.env, [INTL_LOCALES_ENV]: app.config.locales.join(',') } },
  );
  task.progress('Hermes');
  const hermesc = await locateHermesc(task, app.packages.reactNative);
  await task.run([hermesc, '-emit-binary', '-O', '-w', '-out', bundle.bytecode, bundle.javascript]);
  return bundle;
}
