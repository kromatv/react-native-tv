import { z } from 'zod';

/** Where the preview service fetches Samsung's preview JSON while the app is closed. */
export interface PreviewSource {
  readonly url: string;
  /** Sent with each fetch, so only to an `https` URL. */
  readonly headers?: Readonly<Record<string, string>>;
}

const sourceSchema = z
  .object({
    url: z.url({ protocol: /^https?$/ }),
    headers: z.record(z.string(), z.string()).optional(),
  })
  .strict()
  .refine(
    (source) =>
      Object.keys(source.headers ?? {}).length === 0 || new URL(source.url).protocol === 'https:',
    { message: 'headers go to an https url only', path: ['headers'] },
  );

/**
 * `source` once checked: an `http` or `https` URL, string headers, and headers
 * only with `https`, since they usually carry a credential. Throws otherwise.
 */
export function parsePreviewSource(source: PreviewSource): PreviewSource {
  return sourceSchema.parse(source);
}
