import type { SdbConnection } from './sdb-connection.ts';

const DEFAULT_LIMIT = 64 * 1024;
const DEFAULT_TIMEOUT_MS = 30_000;

export interface ShellOptions {
  readonly limit?: number;
  /** How long the TV may stay silent before the command counts as hung. */
  readonly timeoutMs?: number;
  readonly onLine?: (line: string) => void;
}

/** Runs one command through the `shell:` service and returns its output, cut at `limit` bytes. */
export async function runShell(
  connection: SdbConnection,
  command: string,
  { limit = DEFAULT_LIMIT, timeoutMs = DEFAULT_TIMEOUT_MS, onLine }: ShellOptions = {},
): Promise<string> {
  const stream = await connection.openStream(`shell:${command}`);
  let output = '';
  let partial = '';
  try {
    for await (const chunk of stream.chunks(timeoutMs)) {
      const text = chunk.subarray(0, Math.max(0, limit - output.length)).toString('utf8');
      output += text;
      const lines = (partial + text).split(/\r?\n/);
      partial = lines.pop() ?? '';
      for (const line of lines) onLine?.(line);
    }
    if (partial !== '') onLine?.(partial);
    return output;
  } finally {
    stream.close();
  }
}
