const STATUS_LINE = /^\[(\d+)\/(\d+)\]/;

/** Turns a Ninja status line (`[12/480] Building CXX object ...`) into `12/480`, or null. */
export function ninjaProgress(line: string): string | null {
  const match = STATUS_LINE.exec(line);
  if (match === null) return null;
  return `${match[1]}/${match[2]}`;
}
