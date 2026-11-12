import { readFileSync } from 'node:fs';
import { z } from 'zod';
import { DEPENDENCIES_FILE } from '../package-paths.ts';

const pinSchema = z.object({
  version: z.string().min(1),
  url: z.url(),
  sha256: z.string().regex(/^[0-9a-f]{64}$/),
});

const dependenciesSchema = z.record(z.string(), pinSchema);

type PinnedDependency = z.infer<typeof pinSchema>;

/** The pin for one dependency in `runtime/dependencies.json`, which CMake reads too. */
export function pinnedDependency(name: string): PinnedDependency {
  const pins = dependenciesSchema.parse(JSON.parse(readFileSync(DEPENDENCIES_FILE, 'utf8')));
  const pin = pins[name];
  if (pin === undefined) throw new Error(`${DEPENDENCIES_FILE} pins no ${name}`);
  return pin;
}

/** The file name CMake's FetchContent saves an archive under, so both share one download. */
export function archiveFileName(name: string, pin: PinnedDependency): string {
  const urlFile = new URL(pin.url).pathname.split('/').at(-1) ?? 'archive';
  return `${name}-${urlFile}`;
}
