import { styleText } from 'node:util';

type StyleFormat = Parameters<typeof styleText>[0];

/** Styles text for a terminal, or returns it untouched when colors are off. */
export type Paint = (format: StyleFormat, text: string) => string;

/** Colors follow `NO_COLOR` and `FORCE_COLOR`, then whether the stream is a terminal. */
export function colorsEnabled(stream: NodeJS.WriteStream, env = process.env): boolean {
  if (env.NO_COLOR) return false;
  if (env.FORCE_COLOR) return true;
  return stream.isTTY === true;
}

/** A {@link Paint} that colors only when `enabled`. */
export function painter(enabled: boolean): Paint {
  if (!enabled) return (_format, text) => text;
  return (format, text) => styleText(format, text, { validateStream: false });
}
