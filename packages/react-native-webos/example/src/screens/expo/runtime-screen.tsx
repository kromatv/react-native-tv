import { reloadAppAsync, SharedObject, uuid } from 'expo';
import * as Device from 'expo-device';
import { File, Paths } from 'expo-file-system';
import { attempt, check, CheckList, useChecks, type Check } from './check-list.tsx';
import { fetchChecks } from './fetch-checks.ts';

const UUID_V4 = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const DNS_NAMESPACE = '6ba7b810-9dad-11d1-80b4-00c04fd430c8';

type ProbeEvents = { change: (payload: number) => void };

function sharedObject(value: object): InstanceType<SharedObject<ProbeEvents>> {
  if (!(value instanceof SharedObject)) throw new Error('A File is not an expo SharedObject');
  return value;
}

function sharedObjectId(object: object): unknown {
  return Reflect.get(object, '__expo_shared_object_id__');
}

async function runtimeChecks(): Promise<readonly Check[]> {
  const [core, network] = await Promise.all([coreChecks(), fetchChecks()]);
  return [...core, ...network];
}

async function coreChecks(): Promise<readonly Check[]> {
  const core = globalThis.expo;
  return Promise.all([
    attempt('expoModulesCoreVersion', () => {
      const version: unknown = Reflect.get(core, 'expoModulesCoreVersion');
      const major =
        typeof version === 'object' && version !== null ? Reflect.get(version, 'major') : undefined;
      return check('expoModulesCoreVersion', typeof major === 'number' && major >= 58, version);
    }),
    attempt('uuid.v4', () => {
      const value = uuid.v4();
      return check('uuid.v4', UUID_V4.test(value), value);
    }),
    attempt('uuid.v5 (RFC 4122 DNS namespace)', () => {
      const value = uuid.v5('hello', DNS_NAMESPACE);
      return check(
        'uuid.v5 (RFC 4122 DNS namespace)',
        value === '9342d47a-1bab-5709-9869-c840b2eac501',
        value,
      );
    }),
    attempt('getViewConfig(ExpoImage)', () => {
      const config = core.getViewConfig('ExpoImage');
      const known =
        config !== null &&
        'source' in config.validAttributes &&
        'topLoad' in config.directEventTypes;
      return check(
        'getViewConfig(ExpoImage)',
        known && core.getViewConfig('NoSuchModule') === null,
        config,
      );
    }),
    attempt('shared object id, then release', () => {
      const file = sharedObject(new File(Paths.cache, 'expo-probe.txt'));
      const paired = sharedObjectId(file);
      file.release();
      const released = sharedObjectId(file);
      return check(
        'shared object id, then release',
        typeof paired === 'number' && paired > 0 && released === 0,
        { paired, released },
      );
    }),
    attempt('events on a shared object', () => {
      const file = sharedObject(new File(Paths.cache, 'expo-events.txt'));
      let received: unknown = null;
      const subscription = file.addListener('change', (payload) => {
        received = payload;
      });
      file.emit('change', 42);
      subscription.remove();
      return check('events on a shared object', received === 42, received);
    }),
    attempt('async function settles later', async () => {
      let settled = false;
      const pending = Device.getUptimeAsync().then((uptime) => {
        settled = true;
        return uptime;
      });
      const synchronously = settled;
      const uptime = await pending;
      return check(
        'async function settles later',
        !synchronously && uptime > 0,
        `${Math.round(uptime / 1000)} s up`,
      );
    }),
    attempt('reloadAppAsync rejects', async () => {
      const outcome = await reloadAppAsync('probe').then(
        () => 'resolved',
        (error: unknown) => (error instanceof Error ? `rejected: ${error.message}` : 'rejected'),
      );
      return check('reloadAppAsync rejects', outcome.startsWith('rejected'), outcome);
    }),
  ]);
}

export function RuntimeScreen() {
  return <CheckList checks={useChecks('runtime', runtimeChecks)} />;
}
