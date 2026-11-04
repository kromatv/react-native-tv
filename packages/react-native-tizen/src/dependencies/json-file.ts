import { readFileSync, writeFileSync } from 'node:fs';
import { z } from 'zod';
import { FixableError } from '../errors.ts';

/** A JSON object as read from a file. */
export type JsonObject = Record<string, unknown>;

const MAX_JSON_BYTES = 4 * 1024 * 1024;
const jsonObjectSchema = z.record(z.string(), z.unknown());

/** The JSON object in `file`, or a `FixableError` naming the file when it is not one. */
export function readJsonObject(file: string): JsonObject {
  const text = readFileSync(file, 'utf8');
  if (text.length > MAX_JSON_BYTES) throw new FixableError(`${file} is too large`, `Fix ${file}.`);
  try {
    return jsonObjectSchema.parse(JSON.parse(text));
  } catch {
    throw new FixableError(`${file} is not a JSON object`, `Fix ${file}, then try again.`);
  }
}

/** Writes `value` to `file` with the indentation the file had. */
export function writeJsonObject(file: string, value: JsonObject): void {
  const indent = /^([ \t]+)\S/m.exec(readFileSync(file, 'utf8'))?.[1] ?? '  ';
  writeFileSync(file, `${JSON.stringify(value, null, indent)}\n`);
}

/** The object under `key`, or an empty one when there is none. */
export function objectAt(value: JsonObject, key: string): JsonObject {
  const parsed = jsonObjectSchema.safeParse(value[key]);
  return parsed.success ? parsed.data : {};
}
