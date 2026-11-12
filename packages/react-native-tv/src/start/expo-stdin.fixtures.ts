import { EventEmitter } from 'node:events';
import type { KeyInput, KeyListener } from './expo-key-input.ts';

/** A stand-in for `process.stdin` under `expo start`, with Expo's key handler. */
export class FakeStdin extends EventEmitter implements KeyInput {
  isRaw = false;
  isTTY = true;
  readonly expoKeys: string[] = [];
  readonly handleKeypress: KeyListener = (chunk) => {
    this.expoKeys.push(String(chunk));
  };

  /** Expo starts reading keys, as `KeyPressHandler.startInterceptingKeyStrokes` does. */
  startExpo(): void {
    this.isRaw = true;
    this.on('data', this.handleKeypress);
  }

  /** Expo opens a prompt, as `KeyPressHandler.stopInterceptingKeyStrokes` does. */
  pauseExpo(): void {
    this.off('data', this.handleKeypress);
    this.isRaw = false;
  }

  press(key: string): void {
    this.emit('data', key);
  }
}

/** Runs the work `setImmediate` would, at once. */
export const runNow = (work: () => void): void => {
  work();
};
