import { isDeepStrictEqual } from 'node:util';
import { z } from 'zod';

const PackageJson = z.record(z.string(), z.unknown());

function withoutVersion(text: string): Record<string, unknown> {
  const fields = PackageJson.parse(JSON.parse(text));
  delete fields.version;
  return fields;
}

/** Whether two package.json texts differ in nothing but their "version" field and layout. */
export function onlyVersionChanged(before: string, after: string): boolean {
  return isDeepStrictEqual(withoutVersion(before), withoutVersion(after));
}
