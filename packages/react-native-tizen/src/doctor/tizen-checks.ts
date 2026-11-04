import { basename } from 'node:path';
import { DEVELOPER_MODE_HINT, listDevices } from '../tizen/devices.ts';
import { FixableError } from '../errors.ts';
import { resolveSigning } from '../tizen/signing-identity.ts';
import { locateGccToolchain, locateRootstrap, type TizenSdk } from '../tizen/tizen-sdk.ts';
import { rootstrapFor, type TizenTarget } from '../tizen/target.ts';
import { checking, failed, passed, type CheckResult } from './check-result.ts';

const CONNECT_HINT = `${DEVELOPER_MODE_HINT} Then: rn-tizen devices --connect <tv ip>`;

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
    return passed('Rootstrap', rootstrapFor(target).name);
  });
}

/** The certificates of the active signing profile open with their passwords and have not expired. */
export function checkCertificates(): Promise<CheckResult> {
  return checking('Certificates', async () => {
    try {
      const { name, distributor } = await resolveSigning({});
      const until = new Date(distributor.validTo).toISOString().slice(0, 10);
      return passed('Certificates', `${name} (active), distributor until ${until}`);
    } catch (error) {
      if (!(error instanceof FixableError)) throw error;
      return failed('Certificates', error.message, error.hint);
    }
  });
}

/** The remembered TVs that answer. None is a warning: packaging works without one. */
export function checkDevices(): Promise<CheckResult> {
  return checking('TVs', async () => {
    const statuses = await listDevices();
    const online = statuses.flatMap((status) => (status.state === 'online' ? [status.device] : []));
    if (online.length === 0) {
      const detail =
        statuses.length === 0 ? 'none connected yet' : `none of ${statuses.length} answers`;
      return failed('TVs', detail, CONNECT_HINT, 'warn');
    }
    return passed(
      'TVs',
      online.map((device) => `${device.name || 'TV'} at ${device.serial}`).join(', '),
    );
  });
}
