import { z } from 'zod';
import { UsageError } from '../cli/command.ts';

const jsonObjectSchema = z.record(z.string(), z.unknown());

/** `--params` as the one JSON argument a TV's app manager launches an app with, or undefined. */
export function launchParams(json: string | undefined): string | undefined {
  if (json === undefined) return undefined;
  let parsed: unknown;
  try {
    parsed = JSON.parse(json);
  } catch {
    parsed = undefined;
  }
  const params = jsonObjectSchema.safeParse(parsed);
  if (params.success) return JSON.stringify(params.data);
  throw new UsageError(
    `--params takes a JSON object, e.g. '{"contentTarget":"myapp://item/42"}', got ${json}`,
    'simulator',
  );
}
