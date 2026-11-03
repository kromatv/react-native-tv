const FRAME = /^#(\d+) ([^\s+]+)\+0x([0-9a-f]+)$/;

/** How the runtime's crash reporter starts every report it streams. */
export const NATIVE_CRASH_PREFIX = 'Native crash: ';

/** Where a frame of a native crash report points: a library and an offset in it. */
export interface CrashFrame {
  readonly library: string;
  readonly offset: string;
}

/** Names the function at each frame, in order; null for a frame it cannot name. */
export type FrameResolver = (frames: readonly CrashFrame[]) => readonly (string | null)[];

function parseFrame(line: string): CrashFrame | null {
  const match = FRAME.exec(line.trim());
  if (match === null) return null;
  const [, , library, offset] = match;
  return library !== undefined && offset !== undefined ? { library, offset } : null;
}

/**
 * Rewrites the `#N library+0xoffset` frames of a native crash report the runtime
 * streams, appending the function each one is in. A message without frames, or
 * frames the resolver cannot name, come back as they were.
 */
export function symbolizeCrashReport(message: string, resolve: FrameResolver): string {
  const lines = message.split('\n');
  const frames = lines.map(parseFrame);
  const known = frames.filter((frame): frame is CrashFrame => frame !== null);
  if (known.length === 0) return message;
  const names = resolve(known);
  let next = 0;
  return lines
    .map((line, index) => {
      if (frames[index] === null) return line;
      const name = names[next];
      next += 1;
      return name ? `${line}  ${name}` : line;
    })
    .join('\n');
}
