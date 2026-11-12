import { DeviceEventEmitter, TurboModuleRegistry, type TurboModule } from 'react-native';
import { z } from 'zod';

/** The button going down (`pressIn`) or coming back up (`pressOut`). */
export type MicButtonEvent = 'pressIn' | 'pressOut';

/** Stops a listener, and gives the button back once no listener is left. */
export interface MicButtonSubscription {
  remove(): void;
}

interface NativeMicButton extends TurboModule {
  isAvailable(): boolean;
  addListener(event: string): void;
  removeListener(event: string): void;
}

const EVENT = 'rnTizenMicButton';
const pressSchema = z.object({ type: z.enum(['pressIn', 'pressOut']) });

const native = TurboModuleRegistry.get<NativeMicButton>('RNTizenMicButton');

function addListener(event: MicButtonEvent, listener: () => void): MicButtonSubscription {
  if (native === null) return { remove: () => undefined };
  const subscription = DeviceEventEmitter.addListener(EVENT, (payload: unknown) => {
    const press = pressSchema.safeParse(payload);
    if (press.success && press.data.type === event) listener();
  });
  native.addListener(event);
  let removed = false;
  return {
    remove: () => {
      if (removed) return;
      removed = true;
      subscription.remove();
      native.removeListener(event);
    },
  };
}

/**
 * The Smart Remote's microphone button. While a listener is registered the app
 * holds the button, so Samsung's voice assistant stays closed, and each press
 * arrives as it happens: `pressIn` when the button goes down, `pressOut` when it
 * comes back up. On other platforms and off a TV it is unavailable and does nothing.
 */
export const MicButton = {
  /** Whether the app runs on a Samsung TV. A basic remote has no microphone button. */
  get isAvailable(): boolean {
    return native?.isAvailable() ?? false;
  },
  addListener,
};
