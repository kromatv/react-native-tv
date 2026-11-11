import NativePlatformConstantsAndroid from 'react-native/Libraries/Utilities/NativePlatformConstantsAndroid';
import { selectForWebOS, type PlatformSelectSpec } from './select-for-webos.ts';

type Constants = ReturnType<typeof NativePlatformConstantsAndroid.getConstants>;

let constants: Constants | null = null;

function readConstants(): Constants {
  constants ??= NativePlatformConstantsAndroid.getConstants();
  return constants;
}

const Platform = {
  OS: 'webos' as const,
  get Version(): number {
    return readConstants().Version;
  },
  get constants(): Constants {
    return readConstants();
  },
  get isTesting(): boolean {
    return __DEV__ && readConstants().isTesting;
  },
  get isDisableAnimations(): boolean {
    return readConstants().isDisableAnimations ?? this.isTesting;
  },
  isTV: true,
  isVision: false,
  select<T>(spec: PlatformSelectSpec<T>): T | undefined {
    return selectForWebOS(spec);
  },
};

export default Platform;
