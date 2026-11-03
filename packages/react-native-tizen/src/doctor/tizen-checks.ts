import { basename } from 'node:path';
import type { Task } from '../task.ts';
import { listDevices } from '../tizen/sdb.ts';
import { CERTIFICATE_HINT, listSecurityProfiles } from '../tizen/tizen-cli.ts';
import { locateGccToolchain, locateRootstrap, type TizenSdk } from '../tizen/tizen-sdk.ts';
import { rootstrapName, type TizenTarget } from '../tizen/target.ts';
import { checking, failed, passed, type CheckResult } from './check-result.ts';

const DEVELOPER_MODE =
  "Turn on Developer mode on the TV (Apps, then 12345 on the remote) with this computer's IP, then: rn-tizen devices --connect <tv ip>";

/** The GCC runtime Tizen Studio ships for the target's CPU. */
export function checkGcc(sdk: TizenSdk, target: TizenTarget): Promise<CheckResult> {
  return checking('GCC runtime', () =>
    passed('GCC runtime', basename(locateGccToolchain(sdk, target.arch))),
  );
}

/** The rootstrap for the target's platform and CPU. */
export function checkRootstrap(sdk: TizenSdk, target: TizenTarget): Promise<CheckResult> {
  return checking('Rootstrap', () => {
    locateRootstrap(sdk, target);
    return passed('Rootstrap', rootstrapName(target));
  });
}

/** An active security profile, which signs every package. */
export function checkSecurityProfile(task: Task, sdk: TizenSdk): Promise<CheckResult> {
  return checking('Security profile', async () => {
    const profiles = await listSecurityProfiles(task, sdk);
    const active = profiles.find((profile) => profile.active);
    if (active !== undefined) return passed('Security profile', `${active.name} (active)`);
    const detail = profiles.length === 0 ? 'none' : `${profiles.length} profiles, none active`;
    return failed('Security profile', detail, CERTIFICATE_HINT);
  });
}

/** The TVs sdb sees. None is a warning: packaging works without one. */
export function checkDevices(task: Task, sdk: TizenSdk): Promise<CheckResult> {
  return checking('TVs', async () => {
    const online = (await listDevices(task, sdk)).filter((device) => device.state === 'device');
    if (online.length === 0) return failed('TVs', 'none connected', DEVELOPER_MODE, 'warn');
    return passed(
      'TVs',
      online.map((device) => `${device.name || 'TV'} at ${device.serial}`).join(', '),
    );
  });
}
