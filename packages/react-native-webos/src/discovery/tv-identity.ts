import { z } from 'zod';
import { connectSsap, SSAP_PORTS, type SsapPorts } from '../remote/ssap-client.ts';
import { sayHello } from '../remote/ssap-hello.ts';

const SYSTEM_INFO = 'ssap://system/getSystemInfo';
const TIMEOUT_MS = 2000;

const systemInfoSchema = z.object({ modelName: z.string().max(128).optional() });

/** What an LG TV tells anyone on the network, with no pairing and no prompt on its screen. */
export interface TvIdentity {
  /** The id the TV keeps when its address changes; null when it sent none. */
  readonly uuid: string | null;
  /** The webOS release, such as `11.2.0`; null when the TV did not say. */
  readonly releaseVersion: string | null;
  readonly model: string;
}

/**
 * Asks the TV at `host` what it is through its remote control service, and
 * closes. Null when nothing there speaks the protocol. The serial number the
 * TV also gives out is not read.
 */
export async function fetchTvIdentity(
  host: string,
  ports: SsapPorts = SSAP_PORTS,
  timeoutMs = TIMEOUT_MS,
): Promise<TvIdentity | null> {
  const client = await connectSsap(host, { timeoutMs, ports }).catch(() => null);
  if (client === null) return null;
  try {
    const hello = await sayHello(client, timeoutMs);
    const info = systemInfoSchema.safeParse(
      await client.request(SYSTEM_INFO).catch(() => undefined),
    );
    return { ...hello, model: info.success ? (info.data.modelName ?? '') : '' };
  } catch {
    return null;
  } finally {
    client.close();
  }
}
