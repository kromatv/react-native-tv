import { FixableError } from '../errors.ts';
import { addressText, type TvAddress } from './device-address.ts';
import { readDeviceKey } from './device-key.ts';
import { LunaError, TvCommandError } from './luna.ts';
import {
  openSshSession,
  SshConnectError,
  type SshFailure,
  type SshSession,
} from './ssh-session.ts';

export const DEVELOPER_MODE_HINT =
  "On the TV, install the Developer Mode app from LG's store, sign in with an LG developer account, turn Dev Mode Status on and let the TV restart.";

const KEY_SERVER_HINT =
  'In the Developer Mode app on the TV, turn Key Server on, then run: rn-webos connect';

const CONNECT_FAILURES: Record<SshFailure, (tv: string) => FixableError> = {
  unreachable: (tv) =>
    new FixableError(`No TV answers at ${tv}`, 'Is the TV on, and on the same network?'),
  closed: (tv) =>
    new FixableError(
      `${tv} does not accept SSH: Developer Mode is off, or its session ran out`,
      DEVELOPER_MODE_HINT,
    ),
  silent: (tv) =>
    new FixableError(
      `${tv} accepted the connection but did not finish the SSH handshake`,
      'Restart the TV, then try again.',
    ),
  denied: (tv) =>
    new FixableError(`${tv} refused the key rn-webos has for it`, `${KEY_SERVER_HINT} ${tv}`),
};

/** The {@link FixableError} for a TV that could not be reached, or `error` itself. */
export function connectFailure(address: TvAddress, error: unknown): Error {
  if (error instanceof SshConnectError) return CONNECT_FAILURES[error.reason](addressText(address));
  return error instanceof Error ? error : new Error(String(error));
}

/** Opens an SSH session with the key kept for the TV, failing with what to do when there is none. */
export async function openTvSession(address: TvAddress, timeoutMs?: number): Promise<SshSession> {
  const tv = addressText(address);
  const privateKey = readDeviceKey(address);
  if (privateKey === null) {
    throw new FixableError(`rn-webos has no key for the TV at ${tv}`, `${KEY_SERVER_HINT} ${tv}`);
  }
  try {
    return await openSshSession({ ...address, privateKey }, timeoutMs);
  } catch (error) {
    throw connectFailure(address, error);
  }
}

/** Runs `work` on one SSH connection to the TV, so steps chained in it share the login, and closes it. */
export async function onDevice<T>(
  device: { readonly address: TvAddress },
  work: (session: SshSession) => Promise<T>,
): Promise<T> {
  const session = await openTvSession(device.address);
  try {
    return await work(session);
  } finally {
    session.close();
  }
}

/** What the TV said when a command or a luna call failed, for a hint. */
export function tvSaid(error: unknown): string | null {
  if (error instanceof LunaError) return error.reason;
  if (error instanceof TvCommandError)
    return error.output === '' ? error.message : `${error.message}: ${error.output}`;
  return null;
}
