import { TV_PACKAGE } from '../platforms/tv-platform-packages.ts';
import { z } from 'zod';

/** The name apps list this package's config plugin under in `expo.plugins`. */
export const TV_PLUGIN = TV_PACKAGE;

/** The block that holds the props of react-native-tvos' config plugin, for Apple TV and Android TV. */
export const TVOS_BLOCK = 'tvos';

/** The props every TV platform package takes under the same name, given once for all of them. */
export const SHARED_FIELDS = ['package', 'version', 'icon', 'locales'] as const;

const blockSchema = z.record(z.string(), z.unknown());
const propsSchema = z.record(z.string(), z.unknown());

/** The plugin's props, sorted into what every platform gets and what one platform gets. */
export interface TvProps {
  readonly shared: Readonly<Record<string, unknown>>;
  /** The props of each block, by its key: `tvos`, or the name of a platform. */
  readonly blocks: ReadonlyMap<string, Readonly<Record<string, unknown>>>;
}

/** Thrown for props the plugin cannot use, with what to fix. */
export class TvPropsError extends Error {
  constructor(reason: string) {
    super(`Invalid ${TV_PLUGIN} props: ${reason}`);
    this.name = 'TvPropsError';
  }
}

const sharedFields = new Set<string>(SHARED_FIELDS);

/** Sorts the plugin's props: a shared field stays as given, and every other key must hold a block. */
export function tvProps(props: unknown): TvProps {
  const fields = propsSchema.safeParse(props ?? {});
  if (!fields.success) throw new TvPropsError('expected an object');
  const shared: Record<string, unknown> = {};
  const blocks = new Map<string, Record<string, unknown>>();
  for (const [key, value] of Object.entries(fields.data)) {
    if (sharedFields.has(key)) {
      shared[key] = value;
      continue;
    }
    const block = blockSchema.safeParse(value);
    if (!block.success || Array.isArray(value)) {
      throw new TvPropsError(
        `"${key}" is neither a shared field (${SHARED_FIELDS.join(', ')}) nor the block of a platform`,
      );
    }
    blocks.set(key, block.data);
  }
  return { shared, blocks };
}
