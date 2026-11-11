/** One unit of what a development build streams: a line, and the raw bytes it announced. */
export interface StreamFrame {
  readonly line: string;
  readonly payload: Buffer | null;
}

/** Where a stream's chunks go, in order. */
export interface FrameSink {
  push(chunk: Buffer): void;
  end(): void;
}

interface SplitOptions {
  readonly maxLineBytes: number;
  payloadLength(line: string): number;
  onFrame(frame: StreamFrame): void;
}

interface Payload {
  readonly line: string;
  readonly length: number;
  readonly parts: Buffer[];
  received: number;
}

const NEWLINE = 0x0a;
const EMPTY = Buffer.alloc(0);

/**
 * Splits a byte stream into newline-terminated lines. A line for which
 * `payloadLength` answers N > 0 is followed by exactly N raw bytes, delivered
 * with it. Blank lines are skipped, and a line longer than `maxLineBytes` is
 * cut there.
 */
export function splitFrames(options: SplitOptions): FrameSink {
  let buffered: Buffer = EMPTY;
  let payload: Payload | null = null;

  const emitLine = (bytes: Buffer) => {
    const line = bytes.toString('utf8');
    if (line.trim() === '') return;
    const length = options.payloadLength(line);
    if (length > 0) payload = { line, length, parts: [], received: 0 };
    else options.onFrame({ line, payload: null });
  };

  const fillPayload = (current: Payload) => {
    const take = Math.min(current.length - current.received, buffered.length);
    current.parts.push(buffered.subarray(0, take));
    current.received += take;
    buffered = buffered.subarray(take);
    if (current.received < current.length) return;
    payload = null;
    options.onFrame({ line: current.line, payload: Buffer.concat(current.parts) });
  };

  const drain = () => {
    while (buffered.length > 0) {
      if (payload !== null) {
        fillPayload(payload);
        continue;
      }
      const newline = buffered.indexOf(NEWLINE);
      if (newline === -1) {
        if (buffered.length <= options.maxLineBytes) return;
        emitLine(buffered.subarray(0, options.maxLineBytes));
        buffered = EMPTY;
        return;
      }
      emitLine(buffered.subarray(0, newline));
      buffered = buffered.subarray(newline + 1);
    }
  };

  return {
    push(chunk) {
      buffered = buffered.length === 0 ? chunk : Buffer.concat([buffered, chunk]);
      drain();
    },
    end() {
      if (payload === null && buffered.length > 0) emitLine(buffered);
      buffered = EMPTY;
    },
  };
}
