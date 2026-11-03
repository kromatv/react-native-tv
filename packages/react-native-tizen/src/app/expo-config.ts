import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { z } from 'zod';
import { CommandError, lastLines, runSync } from '../exec.ts';

const MAX_OUTPUT_BYTES = 16 * 1024 * 1024;
const ERROR_LINES = 12;

const READ_CONFIG = `
const { writeFileSync } = require('node:fs');
const { createRequire } = require('node:module');
const { join } = require('node:path');
const [root, visibility, out] = process.argv.slice(1);
try {
  const { getConfig } = createRequire(join(root, 'package.json'))('expo/config');
  const { exp } = getConfig(root, {
    isPublicConfig: visibility === 'public',
    skipSDKVersionRequirement: true,
  });
  writeFileSync(out, JSON.stringify(exp));
} catch (error) {
  process.stderr.write(String(error instanceof Error ? error.message : error));
  process.exit(1);
}
`;

const expoConfigSchema = z.record(z.string(), z.unknown());

/** Which part of the config {@link readExpoConfig} returns. */
export interface ReadExpoConfigOptions {
  /** The public config, as `Constants.expoConfig` sees it, rather than the whole one. */
  readonly isPublicConfig?: boolean;
}

/**
 * The app's Expo config, as `expo/config`'s `getConfig` returns it (`exp`),
 * evaluated by Node, which loads an `app.config.ts` where Bun cannot. Throws an
 * `Error` whose message is the reason Expo gave.
 */
export function readExpoConfig(
  root: string,
  options: ReadExpoConfigOptions = {},
): Record<string, unknown> {
  const visibility = options.isPublicConfig === true ? 'public' : 'all';
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-config-'));
  const out = join(dir, 'config.json');
  try {
    runSync(['node', '-e', READ_CONFIG, root, visibility, out], {
      cwd: root,
      maxBytes: MAX_OUTPUT_BYTES,
    });
    return expoConfigSchema.parse(JSON.parse(readFileSync(out, 'utf8')));
  } catch (error) {
    if (!(error instanceof CommandError)) throw error;
    throw new Error(lastLines(error.output, ERROR_LINES).join('\n') || error.message);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}
