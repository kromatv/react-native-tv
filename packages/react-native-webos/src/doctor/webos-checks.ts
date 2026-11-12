import { webosVersionName } from '../discovery/webos-version.ts';
import { addressText } from '../webos/device-address.ts';
import { CONNECT_HINT, listDevices, type Device } from '../webos/devices.ts';
import { checking, failed, passed, type CheckResult } from './check-result.ts';

function describe(device: Device): string {
  const { modelName, sdkVersion } = device.info;
  const details = [
    modelName === device.name ? '' : modelName,
    sdkVersion === '' ? '' : webosVersionName(sdkVersion),
  ].filter((part) => part !== '');
  const about = details.length === 0 ? '' : ` (${details.join(', ')})`;
  return `${device.name} at ${addressText(device.address)}${about}`;
}

/**
 * The connected TVs that let `rn-webos` in: reachable, Developer Mode on, and
 * a key that still opens them. None is a warning: packaging works without one.
 */
export function checkDevices(): Promise<CheckResult> {
  return checking('TVs', async () => {
    const statuses = await listDevices();
    const online = statuses.flatMap((status) => (status.state === 'online' ? [status.device] : []));
    const offline = statuses.flatMap((status) => (status.state === 'offline' ? [status] : []));
    if (statuses.length === 0) return failed('TVs', 'none connected yet', CONNECT_HINT, 'warn');
    if (online.length > 0) {
      const silent = offline.map((status) => status.known.name).join(', ');
      const rest = silent === '' ? '' : `; not answering: ${silent}`;
      return passed('TVs', `${online.map(describe).join(', ')}${rest}`);
    }
    const reasons = offline.map((status) => status.reason).join('; ');
    const detail = offline.length === 1 ? reasons : `none of ${offline.length} answers: ${reasons}`;
    return failed('TVs', detail, offline[0]?.hint ?? CONNECT_HINT, 'warn');
  });
}
