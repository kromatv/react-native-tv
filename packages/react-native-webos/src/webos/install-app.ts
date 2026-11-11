import { createHash } from 'node:crypto';
import { createReadStream } from 'node:fs';
import { pipeline } from 'node:stream/promises';
import { FixableError } from '../errors.ts';
import { addressText, type TvAddress } from './device-address.ts';
import { installerSettled, installerStatus } from './installer-status.ts';
import { lunaFollow, TvCommandError } from './luna.ts';
import { pushFile } from './push-file.ts';
import type { SshSession } from './ssh-session.ts';
import { onDevice, tvSaid } from './tv-session.ts';

const TEMP_DIR = '/media/developer/temp';
const INSTALL = 'luna://com.webos.appInstallService/dev/install';
// LG's ares CLI sends this id for every package: in Developer Mode the installer takes the id from the package.
const PLACEHOLDER_ID = 'com.ares.defaultName';
const INSTALL_TIMEOUT_MS = 300_000;
const NO_SPACE = /space/i;

type Tv = { readonly address: TvAddress };

async function md5Of(file: string): Promise<string> {
  // oxlint-disable-next-line sonarjs/hashing
  const hash = createHash('md5'); // NOSONAR: compared with the TV's md5sum, as LG's CLI checks a pushed package
  await pipeline(createReadStream(file), hash);
  return hash.digest('hex');
}

async function verify(session: SshSession, remote: string, digest: string): Promise<void> {
  const result = await session.exec(`/usr/bin/md5sum ${remote}`);
  const onTv = /^[0-9a-f]{32}/.exec(result.stdout.trim())?.[0];
  if (result.code !== 0 || onTv === undefined)
    throw new TvCommandError('The TV could not check the package it received', result);
  if (onTv !== digest) throw new TvCommandError('the package arrived damaged', result);
}

function refused(tv: string, error: unknown): Error {
  const reason = tvSaid(error);
  if (reason === null) return error instanceof Error ? error : new Error(String(error));
  const hint = NO_SPACE.test(reason)
    ? 'Developer Mode apps share one small partition on the TV: remove one with rn-webos uninstall <app id>.'
    : 'Check Developer Mode is still on (rn-webos devices), then try again.';
  return new FixableError(`${tv} did not install the package: ${reason}`, hint);
}

/**
 * Pushes an `.ipk` to the TV on an open session and installs it, replacing any
 * earlier install. Returns the package id the TV read from it, or null when it named none.
 */
export async function installOn(
  session: SshSession,
  device: Tv,
  file: string,
  onProgress: (detail: string) => void = () => undefined,
): Promise<string | null> {
  let packageId: string | null = null;
  const digest = await md5Of(file);
  const remote = `${TEMP_DIR}/rn-webos-${digest.slice(0, 10)}.ipk`;
  try {
    const made = await session.exec(`/usr/bin/test -d ${TEMP_DIR} || /bin/mkdir -p ${TEMP_DIR}`);
    if (made.code !== 0) throw new TvCommandError(`The TV has no ${TEMP_DIR}`, made);
    await pushFile(session, file, remote, (sent, total) => {
      onProgress(`sending ${total === 0 ? 100 : Math.floor((sent * 100) / total)}%`);
    });
    await verify(session, remote, digest);
    await lunaFollow(
      session,
      INSTALL,
      { id: PLACEHOLDER_ID, ipkUrl: remote, subscribe: true },
      (reply) => {
        const status = installerStatus(reply);
        if (status !== null) onProgress(status.state);
        packageId = status?.packageId ?? packageId;
        return installerSettled(INSTALL, reply, /installed|^SUCCESS/i);
      },
      INSTALL_TIMEOUT_MS,
    );
    return packageId;
  } catch (error) {
    throw refused(addressText(device.address), error);
  } finally {
    await session.exec(`/bin/rm -f ${remote}`).catch(() => undefined);
  }
}

/**
 * Pushes an `.ipk` to the TV and installs it, replacing any earlier install.
 * `onProgress` hears what the install is doing, such as `sending 40%`.
 */
export async function installApp(
  device: Tv,
  file: string,
  onProgress?: (detail: string) => void,
): Promise<void> {
  await onDevice(device, (session) => installOn(session, device, file, onProgress));
}
