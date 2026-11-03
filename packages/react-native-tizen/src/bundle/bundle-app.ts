import { createRequire } from 'node:module';
import { mkdir, rm } from 'node:fs/promises';
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

/** What {@link bundleApp} wrote. */
export interface AppBundle {
  readonly javascript: string;
  readonly bytecode: string;
  readonly assetsDir: string;
}

function requireIntlData(locales: readonly string[]): void {
  const missing = localesWithoutIntlData(locales);
  if (missing.length === 0) return;
  throw new FixableError(
    `expo.tizen.locales: no Intl data for ${missing.join(', ')}`,
    'Name locales FormatJS has data for, such as fr or pt-PT.',
  );
}

/**
 * Bundles the app for Tizen with Expo's own `export:embed`, through the app's
 * Metro config, with Intl data for the app's locales, then compiles the bundle
 * to Hermes bytecode.
 */
export async function bundleApp(task: Task, app: AppProject, outDir: string): Promise<AppBundle> {
  requireIntlData(app.config.locales);
  const bundle: AppBundle = {
    javascript: join(outDir, 'index.tizen.js'),
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
