import { z } from 'zod';

const entrySchema = z.union([z.string(), z.tuple([z.string()]).rest(z.unknown())]);
const pluginsSchema = z.array(z.unknown());

/** The names of the plugins an Expo config lists by name, as a string or as the first item of an entry. */
export function listedPlugins(plugins: unknown): Set<string> {
  const entries = pluginsSchema.safeParse(plugins);
  const names = new Set<string>();
  if (!entries.success) return names;
  for (const entry of entries.data) {
    const parsed = entrySchema.safeParse(entry);
    if (!parsed.success) continue;
    names.add(typeof parsed.data === 'string' ? parsed.data : parsed.data[0]);
  }
  return names;
}
