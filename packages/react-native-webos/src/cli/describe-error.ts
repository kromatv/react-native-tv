import { basename } from 'node:path';
import { FixableError } from '../errors.ts';
import { CommandError, lastLines, MissingProgramError } from '../exec.ts';
import { UsageError } from './command.ts';
import { installHint } from './install-hints.ts';

const OUTPUT_TAIL = 20;
const USAGE_EXIT_CODE = 2;

interface ErrorReport {
  readonly message: string;
  readonly details: readonly string[];
  readonly hint: string;
  readonly exitCode: number;
}

function usageReport(error: UsageError): ErrorReport {
  const help = error.command === null ? 'rn-tizen --help' : `rn-tizen ${error.command} --help`;
  return {
    message: error.message,
    details: [],
    hint: `${help} lists what it takes.`,
    exitCode: USAGE_EXIT_CODE,
  };
}

function commandReport(error: CommandError): ErrorReport {
  const program = basename(error.command[0] ?? 'A command');
  const status = error.exitCode === null ? 'was killed' : `exited with ${error.exitCode}`;
  return {
    message: `${program} ${status}`,
    details: lastLines(error.output, OUTPUT_TAIL),
    hint: 'Run again with --verbose to see everything it printed.',
    exitCode: 1,
  };
}

/** Turns anything a command threw into what the user sees; only a bug shows its stack. */
export function describeError(error: unknown): ErrorReport {
  if (error instanceof UsageError) return usageReport(error);
  if (error instanceof FixableError) {
    return { message: error.message, details: [], hint: error.hint, exitCode: 1 };
  }
  if (error instanceof MissingProgramError) {
    const program = basename(error.program);
    return {
      message: `${program} is not installed or not on PATH`,
      details: [],
      hint: installHint(program),
      exitCode: 1,
    };
  }
  if (error instanceof CommandError) return commandReport(error);
  const bug = error instanceof Error ? error : new Error(String(error));
  return {
    message: bug.message,
    details: (bug.stack ?? '').split('\n').slice(1),
    hint: 'This looks like a bug in rn-tizen. Please report it with the output above.',
    exitCode: 1,
  };
}
