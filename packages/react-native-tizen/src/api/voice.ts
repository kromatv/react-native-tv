import { DeviceEventEmitter, TurboModuleRegistry, type TurboModule } from 'react-native';
import {
  parseVoiceCommand,
  parseVoiceSearch,
  type VoiceCommand,
  type VoiceScreen,
  type VoiceSearch,
} from './voice-events.ts';

export type { VoiceCommand, VoiceScreen, VoiceSearch } from './voice-events.ts';

/** Stops a listener. */
export interface VoiceSubscription {
  remove(): void;
}

interface NativeVoice extends TurboModule {
  isAvailable(): boolean;
  setScreen(screen: string | null): void;
  addListener(event: string): void;
  removeListener(event: string): void;
}

interface VoiceEvents {
  readonly search: VoiceSearch;
  readonly command: VoiceCommand;
}

const EVENTS = { search: 'rnTizenVoiceSearch', command: 'rnTizenVoiceCommand' } as const;

const PARSERS: { readonly [E in keyof VoiceEvents]: (payload: unknown) => VoiceEvents[E] | null } =
  {
    search: parseVoiceSearch,
    command: parseVoiceCommand,
  };

const native = TurboModuleRegistry.get<NativeVoice>('RNTizenVoice');

function addListener<E extends keyof VoiceEvents>(
  event: E,
  listener: (value: VoiceEvents[E]) => void,
): VoiceSubscription {
  if (native === null) return { remove: () => undefined };
  const parse = PARSERS[event];
  const subscription = DeviceEventEmitter.addListener(EVENTS[event], (payload: unknown) => {
    const value = parse(payload);
    if (value !== null) listener(value);
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
 * Samsung's voice assistant on a TV: what the user says while the app is in
 * front. Without the config plugin's `voice` prop, on other platforms and in the simulator
 * it is unavailable and every method does nothing.
 */
export const Voice = {
  /** Whether Samsung's voice service took the app: false without the config plugin's `voice` prop, off a TV, and in the simulator. */
  get isAvailable(): boolean {
    return native?.isAvailable() ?? false;
  },
  /**
   * What the screen shows, which the assistant reads before each utterance.
   * Null goes back to the default: `player` while a video plays, `list` otherwise.
   */
  setScreen(screen: VoiceScreen | null): void {
    native?.setScreen(screen);
  },
  /**
   * `search` takes "search Blade Runner"; with no listener, the `voice.searchUrl` prop
   * is opened through `Linking`. `command` takes transport commands; with no
   * listener, the playing expo-video player obeys them itself.
   */
  addListener,
};
