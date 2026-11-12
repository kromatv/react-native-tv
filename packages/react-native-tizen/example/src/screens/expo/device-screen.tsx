import Constants from 'expo-constants';
import * as Device from 'expo-device';
import { isAvailableAsync, useKeepAwake } from 'expo-keep-awake';
import { attempt, check, CheckList, useChecks, type Check } from './check-list.tsx';

async function deviceChecks(): Promise<readonly Check[]> {
  return Promise.all([
    attempt('Device.deviceType is TV', () =>
      check(
        'Device.deviceType is TV',
        Device.deviceType === Device.DeviceType.TV,
        Device.deviceType,
      ),
    ),
    attempt('Device.deviceName', () =>
      check('Device.deviceName', Boolean(Device.deviceName), Device.deviceName),
    ),
    attempt('Device.modelName', () =>
      check('Device.modelName', Boolean(Device.modelName), Device.modelName),
    ),
    attempt('Device.osName and osVersion', () =>
      check(
        'Device.osName and osVersion',
        Device.osName === 'Tizen',
        `${Device.osName} ${Device.osVersion}`,
      ),
    ),
    attempt('Device.totalMemory', () =>
      check(
        'Device.totalMemory',
        (Device.totalMemory ?? 0) > 0,
        `${Math.round((Device.totalMemory ?? 0) / 2 ** 20)} MiB`,
      ),
    ),
    attempt('Device.supportedCpuArchitectures', () =>
      check(
        'Device.supportedCpuArchitectures',
        (Device.supportedCpuArchitectures?.length ?? 0) > 0,
        Device.supportedCpuArchitectures,
      ),
    ),
    attempt('Constants.expoConfig', () =>
      check(
        'Constants.expoConfig',
        Constants.expoConfig?.slug === 'react-native-tizen-example',
        Constants.expoConfig?.name ?? null,
      ),
    ),
    attempt('Constants.executionEnvironment', () =>
      check(
        'Constants.executionEnvironment',
        Constants.executionEnvironment === 'bare',
        Constants.executionEnvironment,
      ),
    ),
    attempt('Constants.sessionId', () =>
      check('Constants.sessionId', Constants.sessionId.length === 36, Constants.sessionId),
    ),
    attempt('keep awake is available', async () => {
      const available = await isAvailableAsync();
      return check('keep awake is available', available, available);
    }),
  ]);
}

export function DeviceScreen() {
  useKeepAwake();
  return <CheckList checks={useChecks('device', deviceChecks)} />;
}
