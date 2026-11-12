import { remoteButton } from './remote-keys.ts';
import { openRemote, type OpenRemoteOptions } from './remote-session.ts';

const CONNECT_TIMEOUT_MS = 10_000;
const PAIRING_TIMEOUT_MS = 30_000;
const KEY_INTERVAL_MS = 300;

/** How {@link pressRemoteKeys} reaches the TV; every field has a default. */
export type PressOptions = Partial<Omit<OpenRemoteOptions, 'host'>>;

/**
 * Presses remote keys on the TV at `host`, in order, through its network
 * remote. The first time, the TV asks its viewer to accept the connection.
 * A key is pressed and released at once: the protocol has no way to hold one.
 */
export async function pressRemoteKeys(
  host: string,
  keys: readonly string[],
  options: PressOptions = {},
): Promise<void> {
  const buttons = keys.map(remoteButton);
  const session = await openRemote({
    timeoutMs: CONNECT_TIMEOUT_MS,
    pairingTimeoutMs: PAIRING_TIMEOUT_MS,
    keyIntervalMs: KEY_INTERVAL_MS,
    ...options,
    host,
  });
  try {
    await session.press(buttons);
  } finally {
    session.close();
  }
}
