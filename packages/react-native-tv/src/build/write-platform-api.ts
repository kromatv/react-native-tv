import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';
import { z } from 'zod';
import { platformApiModules } from './platform-api.ts';

const ownManifestSchema = z.looseObject({
  dependencies: z.record(z.string(), z.unknown()).optional(),
});

function readManifest(file: string): unknown {
  try {
    return JSON.parse(readFileSync(file, 'utf8'));
  } catch {
    return null;
  }
}

/**
 * Writes, under `outDir`, one module per API module of each TV platform among
 * the dependencies of the package at `ownManifest`: `tizen/preview.js` and
 * `tizen/preview.d.ts`, each re-exporting the platform package's module.
 * Returns the paths written, without extension.
 */
export function writePlatformApi(ownManifest: string, outDir: string): string[] {
  const own = ownManifestSchema.safeParse(readManifest(ownManifest));
  if (!own.success) return [];
  const fromOwn = createRequire(ownManifest);
  const written: string[] = [];
  for (const name of Object.keys(own.data.dependencies ?? {}).sort((first, second) =>
    first.localeCompare(second),
  )) {
    let manifest: string;
    try {
      manifest = fromOwn.resolve(`${name}/package.json`);
    } catch {
      continue;
    }
    for (const { file, specifier } of platformApiModules(readManifest(manifest))) {
      const target = join(outDir, file);
      mkdirSync(dirname(target), { recursive: true });
      const source = `export * from ${JSON.stringify(specifier)};\n`;
      writeFileSync(`${target}.js`, source);
      writeFileSync(`${target}.d.ts`, source);
      written.push(file);
    }
  }
  return written;
}
