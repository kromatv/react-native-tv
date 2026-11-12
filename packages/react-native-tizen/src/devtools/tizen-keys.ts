import { expoReadsKeys, isExpoKeyHandler, type KeyInput } from './expo-key-input.ts';

/** What the Tizen keys do, given by the plugin's server, replaced in tests. */
export interface TizenKeyActions {
  readonly open: () => Promise<void>;
  readonly choose: () => Promise<void>;
  readonly usage: (verbose: boolean) => void;
  readonly busy: () => void;
  readonly defer: (work: () => void) => void;
}

/**
 * Adds `t` and `shift+t` to the keys of `expo start`, once Expo reads keys: it
 * prints the line Expo's usage lacks, and acts only while Expo's own handler
 * listens, so a key typed into one of Expo's prompts does nothing here.
 */
export function listenForTizenKeys(input: KeyInput, actions: TizenKeyActions): void {
  let listening = false;
  let opening = false;
  const run = (action: () => Promise<void>) => {
    if (opening) {
      actions.busy();
      return;
    }
    opening = true;
    void action().finally(() => {
      opening = false;
    });
  };
  const onKey = (chunk: unknown) => {
    if (!expoReadsKeys(input)) return;
    const key = String(chunk);
    if (key === '?') actions.defer(() => actions.usage(true));
    else if (key === 't') run(actions.open);
    else if (key === 'T') run(actions.choose);
  };
  input.on('newListener', (event, listener) => {
    if (listening || event !== 'data' || !isExpoKeyHandler(listener)) return;
    listening = true;
    actions.usage(false);
    actions.defer(() => input.on('data', onKey));
  });
}
