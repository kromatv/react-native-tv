import { isExpoKeyHandler, type KeyInput, type KeyListener } from './expo-key-input.ts';
import type { WebOSTarget } from './webos-targets.ts';

const ESCAPE = '\u001b';
const CTRL_C = '\u0003';
const MAX_CHOICES = 9;

function listing(targets: readonly WebOSTarget[]): string {
  const lines = targets.map((each, index) => `  ${index + 1}  ${each.label}`);
  return `› Select a webOS TV or the simulator\n${lines.join('\n')}\n  Esc  cancel\n`;
}

/**
 * Asks which target to open, one key per choice, as Expo's `shift+a` asks for
 * a device. Expo's key handler is taken off stdin meanwhile, as Expo does for
 * its own prompts, and put back after; Ctrl+C still reaches it.
 */
export function promptWebOSTarget(
  input: KeyInput,
  targets: readonly WebOSTarget[],
  write: (text: string) => void,
): Promise<string | null> {
  const choices = targets.slice(0, MAX_CHOICES);
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
      const chosen = choices[Number(key) - 1];
      if (key !== ESCAPE && key !== CTRL_C && chosen === undefined) return;
      input.off('data', onKey);
      restore();
      if (key === CTRL_C) for (const handler of expo) handler(key);
      resolve(chosen?.target ?? null);
    };
    input.on('data', onKey);
  });
}
