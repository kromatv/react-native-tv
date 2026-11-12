import { z } from 'zod';
import { readText } from './read-text.ts';

/** A `package.json`: its text, for edits that keep its formatting, and the fields the patch reads. */
export interface AppManifest {
  readonly text: string;
  readonly scripts: Readonly<Record<string, string>>;
  readonly patchedDependencies: Readonly<Record<string, string>>;
  readonly resolutions: Readonly<Record<string, string>>;
}

const MAX_MANIFEST_BYTES = 4 * 1024 * 1024;

const strings = z
  .record(z.string(), z.string().optional().catch(undefined))
  .catch({})
  .transform((members): Record<string, string> =>
    Object.fromEntries(
      Object.entries(members).flatMap(([key, value]) =>
        value === undefined ? [] : [[key, value]],
      ),
    ),
  );

const manifestSchema = z.looseObject({
  scripts: strings,
  patchedDependencies: strings,
  resolutions: strings,
});

/** The `package.json` at `file`, or null when there is none that parses. */
export function readManifest(file: string): AppManifest | null {
  const text = readText(file, MAX_MANIFEST_BYTES);
  if (text === null) return null;
  try {
    const { scripts, patchedDependencies, resolutions } = manifestSchema.parse(JSON.parse(text));
    return { text, scripts, patchedDependencies, resolutions };
  } catch {
    return null;
  }
}
