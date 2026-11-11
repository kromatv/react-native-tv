import type { ZodType } from 'zod';

const TIMEOUT_MS = 1500;
const DEFAULT_MAX_BYTES = 64 * 1024;

async function readBounded(response: Response, maxBytes: number): Promise<string | null> {
  if (Number(response.headers.get('content-length') ?? '0') > maxBytes) return null;
  if (response.body === null) return null;
  const chunks: Uint8Array[] = [];
  let size = 0;
  for await (const chunk of response.body) {
    size += chunk.byteLength;
    if (size > maxBytes) return null;
    chunks.push(chunk);
  }
  return Buffer.concat(chunks).toString('utf8');
}

/** A URL's body as text, or null when it is unreachable, not OK or larger than `maxBytes`. */
export async function fetchText(url: string, maxBytes = DEFAULT_MAX_BYTES): Promise<string | null> {
  try {
    const response = await fetch(url, { signal: AbortSignal.timeout(TIMEOUT_MS) });
    return response.ok ? await readBounded(response, maxBytes) : null;
  } catch {
    return null;
  }
}

/** As {@link fetchText}, parsed as JSON and held to `schema`: anything on the network can answer. */
export async function fetchJson<T>(url: string, schema: ZodType<T>): Promise<T | null> {
  const text = await fetchText(url);
  if (text === null) return null;
  try {
    const parsed = schema.safeParse(JSON.parse(text));
    return parsed.success ? parsed.data : null;
  } catch {
    return null;
  }
}
