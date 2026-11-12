import NativePlatformConstantsAndroid from 'react-native/Libraries/Utilities/NativePlatformConstantsAndroid';
import { selectForTizen, type PlatformSelectSpec } from './select-for-tizen.ts';

type Constants = ReturnType<typeof NativePlatformConstantsAndroid.getConstants>;

let constants: Constants | null = null;

function readConstants(): Constants {
  constants ??= NativePlatformConstantsAndroid.getConstants();
  return constants;
}

const Platform = {
  OS: 'tizen' as const,
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
    return selectForTizen(spec);
  },
};

export default Platform;
