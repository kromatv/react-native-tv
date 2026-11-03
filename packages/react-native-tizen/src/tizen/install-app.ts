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
  return `The distributor certificate must list ${named}, and an update must keep the author certificate of the installed app: uninstall that one from the TV first if another author signed it.`;
}

async function onDevice<T>(device: Device, work: (tv: SdbDevice) => Promise<T>): Promise<T> {
  return withSdbDevice(device.address, work).catch((error: unknown) => {
    throw error instanceof SdbCommandError ? error : connectFailure(device.serial, error);
  });
}

/** Pushes a package to the TV and installs it, replacing any earlier install. */
export function installApp(
  task: Task,
  device: Device,
  tpk: string,
  packageId: string,
): Promise<void> {
  return onDevice(device, async (tv) => {
    try {
      await tv.install(tpk, packageId, ({ phase, percent }) => {
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
  });
}

/** Starts an installed app. Success means the TV started its process, not that it drew. */
export function launchApp(device: Device, appId: string): Promise<void> {
  return onDevice(device, async (tv) => {
    try {
      await tv.launch(appId);
    } catch (error) {
      if (!(error instanceof SdbCommandError)) throw error;
      throw new FixableError(
        `${device.serial} did not launch ${appId}`,
        `The TV said: ${error.output || 'nothing'}`,
      );
    }
  });
}
