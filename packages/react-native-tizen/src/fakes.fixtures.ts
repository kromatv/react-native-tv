import { vi } from 'vitest';
import type { RunOptions } from './exec.ts';
import type { TextOutput } from './paint.ts';
import type { Task } from './task.ts';

/** A {@link TextOutput} that keeps what was written. */
export interface CapturedOutput extends TextOutput {
  text(): string;
}

export function capturedOutput(isTTY = false): CapturedOutput {
  const chunks: string[] = [];
  return {
    isTTY,
    write(text: string) {
      chunks.push(text);
      return true;
    },
    text: () => chunks.join(''),
  };
}

/** A {@link Task} that records the commands it was given and answers them with `answer`. */
export interface RecordingTask extends Task {
  readonly commands: (readonly string[])[];
  readonly options: (Omit<RunOptions, 'quiet'> | undefined)[];
  readonly progressed: string[];
}

export function recordingTask(
  answer: (command: readonly string[]) => string | Promise<string> = () => '',
): RecordingTask {
  const commands: (readonly string[])[] = [];
  const options: (Omit<RunOptions, 'quiet'> | undefined)[] = [];
  const progressed: string[] = [];
  return {
    commands,
    options,
    progressed,
    run: async (command, runOptions) => {
      commands.push(command);
      options.push(runOptions);
      return answer(command);
    },
    progress: (detail) => progressed.push(detail),
  };
}

/** Keeps what the code under test prints to stdout and stderr instead of printing it. */
export function captureProcessOutput(): { stdout(): string; stderr(): string } {
  const out: string[] = [];
  const err: string[] = [];
  vi.spyOn(process.stdout, 'write').mockImplementation((chunk: string | Uint8Array) => {
    out.push(String(chunk));
    return true;
  });
  vi.spyOn(process.stderr, 'write').mockImplementation((chunk: string | Uint8Array) => {
    err.push(String(chunk));
    return true;
  });
  return { stdout: () => out.join(''), stderr: () => err.join('') };
}
