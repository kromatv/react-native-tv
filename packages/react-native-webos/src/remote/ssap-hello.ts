import { z } from 'zod';
import type { SsapClient } from './ssap-client.ts';

const text = z.string().max(256);
const helloSchema = z.object({
  deviceOS: text.optional(),
  deviceOSReleaseVersion: text.optional(),
  deviceUUID: z
    .string()
    .regex(/^[0-9A-Za-z-]{1,64}$/)
    .optional(),
});

/** What a TV says of itself before any pairing, with no prompt on its screen. */
export interface TvHello {
  /** The id the TV keeps across addresses, or null when it sent none. */
  readonly uuid: string | null;
  /** The webOS release, such as `11.2.0`; null when the TV did not say. */
  readonly releaseVersion: string | null;
}

/** Sends `hello` and reads the answer; throws when the TV stays silent. */
export async function sayHello(client: SsapClient, timeoutMs: number): Promise<TvHello> {
  client.send({ id: 'hello', type: 'hello', payload: {} });
  const hello = helloSchema.safeParse((await client.next('hello', timeoutMs)).payload);
  return {
    uuid: hello.success ? (hello.data.deviceUUID ?? null) : null,
    releaseVersion: hello.success ? (hello.data.deviceOSReleaseVersion ?? null) : null,
  };
}
