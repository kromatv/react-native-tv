import { z } from 'zod';
import { lunaCall } from './luna.ts';
import type { SshSession } from './ssh-session.ts';

const SYSTEM_INFO = 'luna://com.webos.service.tv.systemproperty//getSystemInfo';
const KEYS = ['modelName', 'sdkVersion', 'firmwareVersion', 'boardType', 'otaId'];

const text = z.string().max(256).optional();
const infoSchema = z.object({ modelName: text, sdkVersion: text, firmwareVersion: text });

/** What a TV says it is; a field it did not report is empty. */
export interface DeviceInfo {
  readonly modelName: string;
  /** The webOS TV release, such as `11.2.0`. */
  readonly sdkVersion: string;
  readonly firmwareVersion: string;
}

/** What is known of a TV that did not answer the question. */
export const NO_DEVICE_INFO: DeviceInfo = { modelName: '', sdkVersion: '', firmwareVersion: '' };

/** Asks the TV for its model, webOS release and firmware, as `ares-device --system-info` does. */
export async function readDeviceInfo(session: SshSession): Promise<DeviceInfo> {
  const info = await lunaCall(session, SYSTEM_INFO, { keys: KEYS, subscribe: false }, infoSchema);
  return {
    modelName: info.modelName ?? '',
    sdkVersion: info.sdkVersion ?? '',
    firmwareVersion: info.firmwareVersion ?? '',
  };
}
