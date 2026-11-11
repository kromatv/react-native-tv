import { FixableError } from '../errors.ts';
import { SdbCommandError, withSdbDevice, type SdbDevice } from '../sdb/sdb-device.ts';
import type { Task } from '../task.ts';
import { connectFailure, type Device } from './devices.ts';

const FAILURE_REASON = /install failed\[[^\]]*\],?\s*reason:\s*(.+)/;
const SIGNATURE_PROBLEM = /certificate|signature/i;

/** What the TV said an install failed for: its `reason:`, else its last line. */
export function installFailureReason(output: string): string {
  const reason = FAILURE_REASON.exec(output)?.[1]?.trim();
  if (reason !== undefined && reason !== '') return reason;
  const lines = output.split(/\r?\n/);
  for (let index = lines.length - 1; index >= 0; index--) {
    const line = lines[index]?.trim() ?? '';
    if (line !== '') return line;
  }
  return 'no output';
}

async function signingHint(tv: SdbDevice): Promise<string> {
  const duid = await tv.duid().catch(() => null);
  const named = duid === null ? "this TV's DUID" : `this TV's DUID, ${duid}`;
  return `The distributor certificate must list ${named}, and an update must keep the author certificate of the installed app: uninstall that one from the TV first (rn-tizen uninstall <package id>) if another author signed it.`;
}

/** Runs `work` on one sdb connection to the TV, so steps chained in it never wait for the TV to let the last one go. */
export function onDevice<T>(device: Device, work: (tv: SdbDevice) => Promise<T>): Promise<T> {
  return withSdbDevice(device.address, work).catch((error: unknown) => {
    throw error instanceof SdbCommandError ? error : connectFailure(device.serial, error);
  });
}

/** Refuses a package whose distributor certificate does not list this TV, before it is sent. */
async function checkDuid(tv: SdbDevice, device: Device, listed: readonly string[]): Promise<void> {
  if (listed.length === 0) return;
  const duid = await tv.duid().catch(() => null);
  if (duid === null || listed.includes(duid)) return;
  throw new FixableError(
    `The distributor certificate does not list ${device.serial}, DUID ${duid}`,
    `It lists ${listed.join(', ')}. Add this TV to the Samsung certificate in Tizen Studio's Certificate Manager, or sign with the profile made for it.`,
  );
}

/** What an install sends: the package file, the id the TV installs it under, the DUIDs its certificate lists. */
export interface InstallRequest {
  readonly file: string;
  readonly installId: string;
  readonly duids?: readonly string[];
}

/** Pushes a package to the TV on an open connection and installs it, replacing any earlier install. */
export async function installOn(
  tv: SdbDevice,
  task: Pick<Task, 'progress'>,
  device: Device,
  request: InstallRequest,
): Promise<void> {
  await checkDuid(tv, device, request.duids ?? []);
  try {
    await tv.install(request.file, request.installId, ({ phase, percent }) => {
      task.progress(phase === 'push' ? `sending ${percent}%` : `installing ${percent}%`);
    });
  } catch (error) {
    if (!(error instanceof SdbCommandError)) throw error;
    const reason = installFailureReason(error.output);
    const hint = SIGNATURE_PROBLEM.test(reason)
      ? await signingHint(tv)
      : 'Check the TV is in Developer mode and has room for the package.';
    throw new FixableError(`${device.serial} refused the package: ${reason}`, hint);
  }
}

/** Pushes a package to the TV and installs it, replacing any earlier install. */
export function installApp(
  task: Pick<Task, 'progress'>,
  device: Device,
  request: InstallRequest,
): Promise<void> {
  return onDevice(device, (tv) => installOn(tv, task, device, request));
}

/** Removes an installed package and its data from the TV. */
export function uninstallApp(device: Device, packageId: string): Promise<void> {
  return onDevice(device, async (tv) => {
    try {
      await tv.uninstall(packageId);
    } catch (error) {
      if (!(error instanceof SdbCommandError)) throw error;
      throw new FixableError(
        `${device.serial} did not uninstall ${packageId}`,
        `The TV said: ${error.output || 'nothing'}`,
      );
    }
  });
}

/** Starts an installed app on an open connection. Success means the TV started its process, not that it drew. */
export async function launchOn(tv: SdbDevice, device: Device, appId: string): Promise<void> {
  try {
    await tv.launch(appId);
  } catch (error) {
    if (!(error instanceof SdbCommandError)) throw error;
    throw new FixableError(
      `${device.serial} did not launch ${appId}`,
      `The TV said: ${error.output || 'nothing'}`,
    );
  }
}

/** Starts an installed app. Success means the TV started its process, not that it drew. */
export function launchApp(device: Device, appId: string): Promise<void> {
  return onDevice(device, (tv) => launchOn(tv, device, appId));
}
