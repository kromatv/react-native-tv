import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname } from 'node:path';
import { z } from 'zod';

/** The Expo CLI an app runs: `@expo/cli`, as its `expo` resolves it. */
export interface ExpoCliPackage {
  readonly dir: string;
  readonly version: string;
  /** The module `expo`'s command requires, which holds the command table. */
  readonly entry: string;
}

const manifestSchema = z.looseObject({ version: z.string().max(64) });

/** The Expo CLI of the app at `projectRoot`, or null when the app has no `expo` to resolve one from. */
export function expoCliPackage(projectRoot: string): ExpoCliPackage | null {
  try {
    const fromApp = createRequire(`${projectRoot}/package.json`);
    const fromExpo = createRequire(fromApp.resolve('expo/package.json'));
    const manifest = fromExpo.resolve('@expo/cli/package.json');
    const { version } = manifestSchema.parse(JSON.parse(readFileSync(manifest, 'utf8')));
    return { dir: dirname(manifest), version, entry: fromExpo.resolve('@expo/cli') };
  } catch {
    return null;
  }
}
