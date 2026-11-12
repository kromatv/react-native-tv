import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { z } from 'zod';
import { objectAt, readJsonObject, writeJsonObject } from './json-file.ts';

const lockSchema = z.looseObject({
  packages: z.record(z.string(), z.looseObject({ name: z.string().optional() })).optional(),
});

const REACT_NATIVE_PATH = /(?:^|\/)node_modules\/react-native$/;

/**
 * Drops `package-lock.json`'s entries for upstream `react-native`, which an
 * alias entry names `react-native-tvos`. npm 11 keeps a locked copy after an
 * override is added, and resolves a dropped entry again through it.
 */
export function unlockUpstreamReactNative(root: string): boolean {
  const file = join(root, 'package-lock.json');
  if (!existsSync(file)) return false;
  const lock = readJsonObject(file);
  const parsed = lockSchema.safeParse(lock);
  if (!parsed.success) return false;
  const upstream = Object.entries(parsed.data.packages ?? {})
    .filter(([path, entry]) => REACT_NATIVE_PATH.test(path) && entry.name === undefined)
    .map(([path]) => path);
  if (upstream.length === 0) return false;
  const packages = Object.entries(objectAt(lock, 'packages')).filter(
    ([path]) => !upstream.includes(path),
  );
  writeJsonObject(file, { ...lock, packages: Object.fromEntries(packages) });
  return true;
}
