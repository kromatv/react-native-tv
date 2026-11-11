/** A key handler on stdin, as Expo's `KeyPressHandler` attaches one. */
export type KeyListener = (chunk: unknown) => void;

/** The part of `process.stdin` the Tizen keys use. */
export interface KeyInput {
  readonly isRaw?: boolean;
  on(event: 'data', listener: KeyListener): unknown;
  on(event: 'newListener', listener: (event: string | symbol, listener: unknown) => void): unknown;
  off(event: 'data', listener: KeyListener): unknown;
  listeners(event: 'data'): readonly unknown[];
}

const EXPO_KEY_HANDLER = 'handleKeypress';

/** Whether `listener` is the handler `expo start` reads its keys with. */
export function isExpoKeyHandler(listener: unknown): listener is KeyListener {
  return typeof listener === 'function' && listener.name === EXPO_KEY_HANDLER;
}

/**
 * Whether Expo is reading keys: its handler is on stdin, in raw mode. Expo
 * takes it off, and leaves raw mode, while one of its prompts is open.
 */
export function expoReadsKeys(input: KeyInput): boolean {
  return input.isRaw === true && input.listeners('data').some(isExpoKeyHandler);
}
