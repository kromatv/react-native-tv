import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { z } from 'zod';
import { RUNTIME_DIR } from '../package-paths.ts';

/** Where the runtime keeps the Tizen implementations it ships for other packages. */
export const BUNDLED_IMPLEMENTATIONS_DIR = join(RUNTIME_DIR, 'host', 'src');

const manifestSchema = z.object({ package: z.string().min(1) });

/**
 * The Tizen implementations this package ships for packages that have none of
 * their own, by npm name: every directory of `runtime/host/src` holding a
 * `library.json` that names the package, beside the CMakeLists.txt that builds
 * it. Returns each CMakeLists.txt.
 */
export function bundledImplementations(
  dir: string = BUNDLED_IMPLEMENTATIONS_DIR,
): ReadonlyMap<string, string> {
  const found = new Map<string, string>();
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    if (!entry.isDirectory()) continue;
    const manifestPath = join(dir, entry.name, 'library.json');
    if (!existsSync(manifestPath)) continue;
    const manifest = manifestSchema.parse(JSON.parse(readFileSync(manifestPath, 'utf8')));
    found.set(manifest.package, join(dir, entry.name, 'CMakeLists.txt'));
  }
  return found;
}
