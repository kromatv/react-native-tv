import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { addressText, type TvAddress } from './device-address.ts';
import { installerSettled } from './installer-status.ts';
import { lunaCall, lunaFollow, type LunaReply } from './luna.ts';
import type { SshSession } from './ssh-session.ts';
import { onDevice, tvSaid } from './tv-session.ts';

// LG's ares CLI calls launch with this double slash: the method has no category.
const LAUNCH = 'luna://com.webos.applicationManager//launch';
const CLOSE = 'luna://com.webos.applicationManager/dev/closeByAppId';
const REMOVE = 'luna://com.webos.appInstallService/dev/remove';
const REMOVE_TIMEOUT_MS = 120_000;
const APP_ID = /^[a-z0-9.+-]+$/;

const acceptedSchema = z.object({ returnValue: z.literal(true) });
type Tv = { readonly address: TvAddress };

/** The app id as it may go into a command on the TV; LG's own rule for ids. */
export function checkedAppId(appId: string): string {
  if (APP_ID.test(appId)) return appId;
  throw new FixableError(
    `${JSON.stringify(appId)} is not a webOS app id`,
    'An app id holds lowercase letters, digits, periods, plus and minus signs, such as com.example.myapp.',
  );
}

function failure(device: Tv, did: string, error: unknown): Error {
  const said = tvSaid(error);
  if (said === null) return error instanceof Error ? error : new Error(String(error));
  return new FixableError(`${addressText(device.address)} did not ${did}`, `The TV said: ${said}`);
}

/** Starts an installed app on an open session. Success means the TV accepted the launch, not that the app drew. */
export async function launchOn(session: SshSession, device: Tv, appId: string): Promise<void> {
  const params = { id: checkedAppId(appId), subscribe: false, params: { displayAffinity: 0 } };
  await lunaCall(session, LAUNCH, params, acceptedSchema).catch((error: unknown) => {
    throw failure(device, `launch ${appId}`, error);
  });
}

/** Starts an installed app. Success means the TV accepted the launch, not that the app drew. */
export function launchApp(device: Tv, appId: string): Promise<void> {
  return onDevice(device, (session) => launchOn(session, device, appId));
}

/** Closes a running app that was installed in Developer Mode. */
export function closeApp(device: Tv, appId: string): Promise<void> {
  const params = { id: checkedAppId(appId), subscribe: false, params: { displayAffinity: 0 } };
  return onDevice(device, async (session) => {
    await lunaCall(session, CLOSE, params, acceptedSchema).catch((error: unknown) => {
      throw failure(device, `close ${appId}`, error);
    });
  });
}

/** Removes an installed app and its data from the TV. */
export function uninstallApp(device: Tv, appId: string): Promise<void> {
  const params = { id: checkedAppId(appId), subscribe: true };
  return onDevice(device, async (session) => {
    const removed = (reply: LunaReply) => installerSettled(REMOVE, reply, /removed|^SUCCESS/i);
    await lunaFollow(session, REMOVE, params, removed, REMOVE_TIMEOUT_MS).catch(
      (error: unknown) => {
        throw failure(device, `uninstall ${appId}`, error);
      },
    );
  });
}
