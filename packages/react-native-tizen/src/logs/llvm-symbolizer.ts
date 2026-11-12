import { existsSync } from 'node:fs';
import { basename } from 'node:path';
import { findLlvmTools } from '../doctor/find-program.ts';
import { runSync } from '../exec.ts';
import type { LogEntry } from './log-line.ts';
import {
  type CrashFrame,
  type FrameResolver,
  NATIVE_CRASH_PREFIX,
  symbolizeCrashReport,
} from './native-crash.ts';

const SYMBOLIZE_TIMEOUT_MS = 10_000;

function describeLocation(functionName: string, location: string): string | null {
  if (functionName === '??' || functionName === '') return null;
  const file = location.replace(/:0:0$/, '');
  return file.startsWith('??') ? functionName : `${functionName} at ${file}`;
}

function parseSymbolizerOutput(output: string): (string | null)[] {
  return output
    .trim()
    .split(/\n\s*\n/)
    .map((block) => {
      const [functionName = '', location = ''] = block.split('\n');
      return describeLocation(functionName.trim(), location.trim());
    });
}

function symbolize(
  symbolizer: string,
  library: string,
  frames: readonly CrashFrame[],
): (string | null)[] {
  const addresses = frames.map((frame) => `0x${frame.offset}`);
  try {
    const output = runSync(
      [symbolizer, '--obj', library, '--no-inlines', '--demangle', ...addresses],
      { timeoutMs: SYMBOLIZE_TIMEOUT_MS },
    );
    return parseSymbolizerOutput(output);
  } catch {
    return [];
  }
}

/**
 * Resolves frames in `library` (an unstripped build of the library the TV
 * runs) with `llvm-symbolizer`; frames in other libraries stay unnamed. Null
 * when the library or llvm-symbolizer is missing.
 */
export function llvmFrameResolver(library: string): FrameResolver | null {
  const symbolizer = findLlvmTools()['llvm-symbolizer'];
  if (symbolizer === null || !existsSync(library)) return null;
  const name = basename(library);
  return (frames: readonly CrashFrame[]) => {
    const own = frames.filter((frame) => frame.library === name);
    if (own.length === 0) return frames.map(() => null);
    const names = symbolize(symbolizer, library, own);
    let next = 0;
    return frames.map((frame) => {
      if (frame.library !== name) return null;
      const resolved = names[next] ?? null;
      next += 1;
      return resolved;
    });
  };
}

/**
 * A log transform that names the frames of native crash reports from `library`.
 * The library is read the first time a crash arrives, so it may still be
 * building when the transform is made.
 */
export function crashSymbolizer(library: string): (entry: LogEntry) => LogEntry {
  let resolve: FrameResolver | null | undefined;
  return (entry) => {
    if (!entry.message.startsWith(NATIVE_CRASH_PREFIX)) return entry;
    resolve ??= llvmFrameResolver(library);
    return resolve === null
      ? entry
      : { ...entry, message: symbolizeCrashReport(entry.message, resolve) };
  };
}
