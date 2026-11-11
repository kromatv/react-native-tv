import 'react-native';

declare module 'react-native' {
  export interface HWEvent {
    eventType: string;
    eventKeyAction?: number;
  }

  export const useTVEventHandler: (handleEvent: (event: HWEvent) => void) => void;
}
