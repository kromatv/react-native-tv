import { FixableError } from '../errors.ts';
import { addressText, DEVELOPER_SSH_PORT, type TvAddress } from './device-address.ts';
import { NO_DEVICE_INFO, readDeviceInfo, type DeviceInfo } from './device-info.ts';
import { saveDeviceKey } from './device-key.ts';
import type { Device } from './devices.ts';
import { decryptDeveloperKey, fetchDeveloperKey } from './key-server.ts';
import { isDeviceName, readKnownDevices, rememberDevice, sameAddress } from './known-devices.ts';
import { openSshSession, SshConnectError } from './ssh-session.ts';
import { connectFailure } from './tv-session.ts';

const LOGIN_TIMEOUT_MS = 15_000;

/** A TV and the unprotected key its key server gave out, to keep under a name. */
export interface RegisterRequest {
  readonly address: TvAddress;
  readonly privateKey: string;
  /** Default: the name the TV already goes by, else its model. */
  readonly name?: string;
}

async function loginInfo(address: TvAddress, privateKey: string): Promise<DeviceInfo> {
  let session;
  try {
    session = await openSshSession({ ...address, privateKey }, LOGIN_TIMEOUT_MS);
  } catch (error) {
    if (error instanceof SshConnectError && error.reason === 'denied') {
      throw new FixableError(
        `The TV at ${addressText(address)} served a key its own SSH server refuses`,
        'Check the address is the TV whose Developer Mode app shows the passphrase, then try again.',
      );
    }
    throw connectFailure(address, error);
  }
  try {
    return await readDeviceInfo(session).catch(() => NO_DEVICE_INFO);
  } finally {
    session.close();
  }
}

function defaultName(info: DeviceInfo, address: TvAddress): string {
  return isDeviceName(info.modelName) ? info.modelName : `tv-${address.host.replaceAll('.', '-')}`;
}

/** Logs in to the TV with `privateKey` to prove it works, then keeps the key and the TV's name. */
export async function registerTv(request: RegisterRequest): Promise<Device> {
  const { address, privateKey } = request;
  if (request.name !== undefined && !isDeviceName(request.name)) {
    throw new FixableError(
      `${JSON.stringify(request.name)} cannot name a TV`,
      'A name holds letters, digits, periods, underscores and minus signs, and is not an IP address.',
    );
  }
  const info = await loginInfo(address, privateKey);
  const known = await readKnownDevices();
  const before = known.find((tv) => sameAddress(tv, address));
  const name = request.name ?? before?.name ?? defaultName(info, address);
  const namesake = known.find((tv) => tv.name === name && !sameAddress(tv, address));
  if (namesake !== undefined) {
    throw new FixableError(
      `The TV at ${addressText(namesake)} is already named ${name}`,
      'Give this one another name with --name.',
    );
  }
  saveDeviceKey(address, privateKey);
  await rememberDevice({ name, ...address });
  return { address, name, info };
}

/** What {@link connectTv} needs: where the TV is and the passphrase its Developer Mode app shows. */
export interface ConnectRequest {
  readonly host: string;
  readonly passphrase: string;
  readonly name?: string;
  /** The TV's ports, 9922 and 9991 unless a test says otherwise. */
  readonly port?: number;
  readonly keyServerPort?: number;
}

/**
 * Sets a TV up for every other call: fetches its SSH key while Key Server is
 * on, opens it with the passphrase, logs in once and remembers the TV.
 */
export async function connectTv(request: ConnectRequest): Promise<Device> {
  const pem = await fetchDeveloperKey(request.host, request.keyServerPort);
  return registerTv({
    address: { host: request.host, port: request.port ?? DEVELOPER_SSH_PORT },
    privateKey: decryptDeveloperKey(pem, request.passphrase),
    name: request.name,
  });
}
