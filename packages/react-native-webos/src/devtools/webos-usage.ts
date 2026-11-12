import type { Paint } from '../paint.ts';

function item(paint: Paint, key: string, text: string): string {
  const press = key.length === 1 ? 'Press ' : '';
  return `› ${press}${paint('bold', key)} ${paint('dim', '│')} ${text}`;
}

/**
 * The lines `t` adds to Expo's usage, in Expo's own format: one, or with
 * `shift+t` as well for the full list `?` prints.
 */
export function webosUsage(paint: Paint, verbose: boolean): string {
  const lines = [item(paint, 't', 'open on webOS TV')];
  if (verbose) lines.push(item(paint, 'shift+t', 'select a webOS TV or the simulator'));
  return `${lines.join('\n')}\n`;
}
