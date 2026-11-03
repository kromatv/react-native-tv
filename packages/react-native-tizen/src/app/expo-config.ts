import { spawnSync } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { z } from 'zod';

const MAX_OUTPUT_BYTES = 16 * 1024 * 1024;

const READ_CONFIG = `
const { writeFileSync } = require('node:fs');
const { createRequire } = require('node:module');
const { join } = require('node:path');
const [root, visibility, out] = process.argv.slice(1);
const { getConfig } = createRequire(join(root, 'package.json'))('expo/config');
const { exp } = getConfig(root, {
  isPublicConfig: visibility === 'public',
  skipSDKVersionRequirement: true,
});
writeFileSync(out, JSON.stringify(exp));
`;

const expoConfigSchema = z.record(z.string(), z.unknown());

/** Which part of the config {@link readExpoConfig} returns. */
export interface ReadExpoConfigOptions {
  /** The public config, as `Constants.expoConfig` sees it, rather than the whole one. */
  readonly isPublicConfig?: boolean;
}

function lastLines(text: string, count: number): string {
  return text.trim().split('\n').slice(-count).join('\n');
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
    const result = spawnSync('node', ['-e', READ_CONFIG, root, visibility, out], {
      cwd: root,
      encoding: 'utf8',
      maxBuffer: MAX_OUTPUT_BYTES,
      stdio: ['ignore', 'pipe', 'pipe'],
    });
    if (result.error !== undefined) throw result.error;
    if (result.status !== 0) {
      throw new Error(lastLines(result.stderr, 12) || `node exited with ${String(result.status)}`);
    }
    return expoConfigSchema.parse(JSON.parse(readFileSync(out, 'utf8')));
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}
