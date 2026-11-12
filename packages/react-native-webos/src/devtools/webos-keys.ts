import { expoReadsKeys, isExpoKeyHandler, type KeyInput } from './expo-key-input.ts';

/** What the webOS keys do, given by the plugin's server, replaced in tests. */
export interface WebOSKeyActions {
  /** Whether another package handles the TV keys of this `expo start` for every platform. */
  readonly claimed: () => boolean;
  readonly open: () => Promise<void>;
  readonly choose: () => Promise<void>;
  readonly usage: (verbose: boolean) => void;
  readonly busy: () => void;
  readonly defer: (work: () => void) => void;
}

/**
 * Adds `t` and `shift+t` to the keys of `expo start`, once Expo reads keys: it
 * prints the line Expo's usage lacks, and acts only while Expo's own handler
 * listens, so a key typed into one of Expo's prompts does nothing here. It
 * adds nothing when the keys are claimed by then.
 */
export function listenForWebOSKeys(input: KeyInput, actions: WebOSKeyActions): void {
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
    if (actions.claimed()) return;
    actions.usage(false);
    actions.defer(() => input.on('data', onKey));
  });
}
