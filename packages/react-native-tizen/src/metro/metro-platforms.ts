import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { CommandError, lastLines, runSync } from '../exec.ts';
import { TIZEN_PLATFORM } from './with-tizen.ts';

const READ_PLATFORMS = `
const { writeFileSync } = require('node:fs');
const { createRequire } = require('node:module');
const { join } = require('node:path');
const [root, out] = process.argv.slice(1);
const fail = (error) => {
  process.stderr.write(String(error instanceof Error ? error.message : error));
  process.exit(1);
};
try {
  const fromApp = createRequire(join(root, 'package.json'));
  const fromExpo = createRequire(fromApp.resolve('expo/package.json'));
  const { loadUserConfig } = fromApp('expo/metro-config');
  const { getMetroServerRoot } = fromExpo('@expo/config/paths');
  loadUserConfig({ projectRoot: root, serverRoot: getMetroServerRoot(root) }).then(
    (config) => writeFileSync(out, JSON.stringify(config.resolver.platforms ?? [])),
    fail,
  );
} catch (error) {
  fail(error);
}
`;

const platformsSchema = z.array(z.string().max(64)).max(64);
const MAX_OUTPUT_BYTES = 4 * 1024 * 1024;
const MAX_PLATFORMS_BYTES = 8 * 1024;
const ERROR_LINES = 12;

/**
 * The platforms the app's Metro config resolves, loaded as `expo start` loads
 * it, with Expo's `loadUserConfig`. Throws an `Error` whose message is the
 * reason Expo gave when the config does not load.
 */
export function metroPlatforms(root: string): string[] {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-metro-'));
  const out = join(dir, 'platforms.json');
  try {
    runSync(['node', '-e', READ_PLATFORMS, root, out], { cwd: root, maxBytes: MAX_OUTPUT_BYTES });
    const text = readFileSync(out, 'utf8');
    if (text.length > MAX_PLATFORMS_BYTES)
      throw new Error('The Metro config lists too many platforms');
    return platformsSchema.parse(JSON.parse(text));
  } catch (error) {
    if (!(error instanceof CommandError)) throw error;
    const reason = lastLines(error.output, ERROR_LINES).join('\n') || error.message;
    throw new Error(`Could not load the app's Metro config: ${reason}`);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}

/** The reason the app's Metro config cannot bundle for Tizen, or null when `withTizen` wraps it. */
export function tizenMetroIssue(root: string): FixableError | null {
  if (metroPlatforms(root).includes(TIZEN_PLATFORM)) return null;
  return new FixableError(
    `The app's Metro config does not know the ${TIZEN_PLATFORM} platform`,
    'Run npx rn-tizen init: it wraps the Metro config with withTizen.',
  );
}
