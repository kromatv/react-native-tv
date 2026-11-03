import { z } from 'zod';

/** A tile's shape on the home screen; Samsung's default is `1:1`. */
export type TileRatio = '16:9' | '4:3' | '1:1' | '2:3';

/** One tile of a Smart Hub preview. */
export interface PreviewTile {
  readonly title?: string;
  readonly subtitle?: string;
  /** PNG or JPEG over http(s), at most 360 KB, 250 to 360 px tall in the ratio's range. */
  readonly image: string;
  readonly ratio?: TileRatio;
  /** Opened through `Linking` when the tile is pressed; unique across the preview. */
  readonly url: string;
  /** Shows a Play icon on the tile. */
  readonly playable?: boolean;
  readonly showFrom?: Date;
  readonly showUntil?: Date;
}

/** A titled row of tiles. */
export interface PreviewSection {
  readonly title?: string;
  readonly tiles: readonly PreviewTile[];
}

/** Samsung's caps on one preview: at most 40 tiles, at least one per section. */
export const MAX_PREVIEW_TILES = 40;

const RATIOS = { '16:9': '16by9', '4:3': '4by3', '1:1': '1by1', '2:3': '2by3' } as const;

const tileSchema = z
  .object({
    title: z.string().min(1).optional(),
    subtitle: z.string().min(1).optional(),
    image: z.url({ protocol: /^https?$/, error: 'must be an http or https URL' }),
    ratio: z.enum(['16:9', '4:3', '1:1', '2:3']).optional(),
    url: z
      .string()
      .regex(/^[a-z][a-z0-9+.-]*:\S+$/i, 'must be a URL with a scheme, such as myapp://item/42'),
    playable: z.boolean().optional(),
    showFrom: z.date().optional(),
    showUntil: z.date().optional(),
  })
  .strict()
  .refine((tile) => !tile.showFrom || !tile.showUntil || tile.showFrom < tile.showUntil, {
    message: 'showFrom must come before showUntil',
  });

const sectionSchema = z
  .object({
    title: z.string().min(1).optional(),
    tiles: z.array(tileSchema).min(1, 'a section needs at least one tile'),
  })
  .strict();

const sectionsSchema = z
  .array(sectionSchema)
  .min(1, 'a preview needs at least one section')
  .refine((sections) => sections.flatMap((section) => section.tiles).length <= MAX_PREVIEW_TILES, {
    message: `a preview holds at most ${MAX_PREVIEW_TILES} tiles`,
  })
  .refine(
    (sections) => {
      const urls = sections.flatMap((section) => section.tiles.map((tile) => tile.url));
      return new Set(urls).size === urls.length;
    },
    { message: 'every tile needs its own url' },
  );

/** Samsung's preview JSON, as `webapis.preview.setPreviewData` takes it. */
export interface SamsungPreview {
  readonly sections: readonly {
    readonly title?: string;
    readonly position: number;
    readonly tiles: readonly Record<string, string | number | boolean>[];
  }[];
}

/** Thrown when sections break one of Samsung's rules; the message names each one. */
export class PreviewValidationError extends Error {
  override readonly name = 'PreviewValidationError';
}

function seconds(date: Date): number {
  return Math.floor(date.getTime() / 1000);
}

function samsungTile(tile: z.infer<typeof tileSchema>, position: number) {
  return {
    ...(tile.title === undefined ? {} : { title: tile.title }),
    ...(tile.subtitle === undefined ? {} : { subtitle: tile.subtitle }),
    image_url: tile.image,
    image_ratio: RATIOS[tile.ratio ?? '1:1'],
    action_data: JSON.stringify({ url: tile.url }),
    is_playable: tile.playable ?? false,
    ...(tile.showFrom === undefined ? {} : { display_from: seconds(tile.showFrom) }),
    ...(tile.showUntil === undefined ? {} : { display_until: seconds(tile.showUntil) }),
    position,
  };
}

/**
 * Checks sections against Samsung's rules and writes them as Samsung's preview
 * JSON, each tile's `url` in its `action_data`. Throws a
 * {@link PreviewValidationError} naming every broken rule.
 */
export function toSamsungPreview(sections: readonly PreviewSection[]): SamsungPreview {
  const parsed = sectionsSchema.safeParse(sections);
  if (!parsed.success) {
    const issues = parsed.error.issues.map((issue) => {
      const path = issue.path.map(String).join('.');
      return path ? `${path}: ${issue.message}` : issue.message;
    });
    throw new PreviewValidationError(issues.join('\n'));
  }
  return {
    sections: parsed.data.map((section, position) => ({
      ...(section.title === undefined ? {} : { title: section.title }),
      position,
      tiles: section.tiles.map(samsungTile),
    })),
  };
}
