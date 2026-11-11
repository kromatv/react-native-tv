import { requireOptionalNativeModule } from 'expo';

export type LaunchParameters = Readonly<Record<string, unknown>>;

interface Size {
  readonly width: number;
  readonly height: number;
}

interface ExpoWebOSModule {
  readonly modelName: string;
  readonly platformVersion: string;
  readonly buildString: string;
  readonly locale: string;
  getScreenSize(): Size;
  getPanelSize(): Size;
  getLaunchParameters(): LaunchParameters;
}

export const expoWebOS = requireOptionalNativeModule<ExpoWebOSModule>('ExpoWebOS');

/** The event the host emits when the TV launches the running app again. */
export const RELAUNCH_EVENT = 'webOSRelaunch';

export interface RelaunchEvent {
  readonly parameters: LaunchParameters;
}
