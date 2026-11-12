import { isExpoKeyHandler, type KeyInput, type KeyListener } from './expo-key-input.ts';

const ESCAPE = '\u001b';
const CTRL_C = '\u0003';

/** How many targets one key each can choose among. */
export const MAX_CHOICES = 9;

function listing(labels: readonly string[]): string {
  const lines = labels.map((label, index) => `  ${index + 1}  ${label}`);
  return `› Select a TV or a simulator\n${lines.join('\n')}\n  Esc  cancel\n`;
}

/**
 * Asks which of `labels` to open, one key per choice, as Expo's `shift+a` asks
 * for a device, and gives its index, or null for Esc. Expo's key handler is
 * taken off stdin meanwhile, as Expo does for its own prompts, and put back
 * after; Ctrl+C still reaches it.
 */
export function promptTvTarget(
  input: KeyInput,
  labels: readonly string[],
  write: (text: string) => void,
): Promise<number | null> {
  const choices = labels.slice(0, MAX_CHOICES);
  const expo = input.listeners('data').filter(isExpoKeyHandler);
  for (const handler of expo) input.off('data', handler);
  const restore = () => {
    for (const handler of expo) {
      if (!input.listeners('data').includes(handler)) input.on('data', handler);
    }
  };
  write(listing(choices));
  return new Promise((resolve) => {
    const onKey: KeyListener = (chunk) => {
      const key = String(chunk);
      const index = Number(key) - 1;
      const chosen = choices[index] !== undefined;
      if (key !== ESCAPE && key !== CTRL_C && !chosen) return;
      input.off('data', onKey);
      restore();
      if (key === CTRL_C) for (const handler of expo) handler(key);
      resolve(chosen ? index : null);
    };
    input.on('data', onKey);
  });
}
