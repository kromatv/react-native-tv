import { basename } from 'node:path';
import { installHint } from '../cli/install-hints.ts';
import { FixableError } from '../errors.ts';
import { MissingProgramError } from '../exec.ts';

/** How a check went. A warning does not fail `doctor`. */
export type CheckStatus = 'ok' | 'warn' | 'fail' | 'skip';

/** One line of `rn-tizen doctor`, with the fix when it did not pass. */
export interface CheckResult {
  readonly title: string;
  readonly status: CheckStatus;
  readonly detail: string;
  readonly fix?: string;
}

/** A check that passed. */
export function passed(title: string, detail: string): CheckResult {
  return { title, status: 'ok', detail };
}

/** A check that failed, or warned, with how to fix it. */
export function failed(
  title: string,
  detail: string,
  fix: string,
  status: 'fail' | 'warn' = 'fail',
): CheckResult {
  return { title, status, detail, fix };
}

/** Runs a check, turning a {@link FixableError} it throws into a failed result. */
export async function checking(
  title: string,
  check: () => Promise<CheckResult> | CheckResult,
): Promise<CheckResult> {
  try {
    return await check();
  } catch (error) {
    if (error instanceof FixableError) return failed(title, error.message, error.hint);
    if (error instanceof MissingProgramError) {
      const program = basename(error.program);
      return failed(title, `${program} is not installed`, installHint(program));
    }
    const reason = error instanceof Error ? error.message : String(error);
    return failed(title, reason, 'Run again with --verbose for details.');
  }
}

const VERSION_WORD = /^v?\d+(?:\.\d+)+$/;

/** The first `x.y[.z]` word in a tool's `--version` output, without a leading `v`, or null. */
export function parseVersion(output: string): string | null {
  const word = output.split(/\s+/).find((candidate) => VERSION_WORD.test(candidate));
  return word === undefined ? null : word.replace(/^v/, '');
}

/** True when `version` is `minimum` or newer, comparing numerically. */
export function isAtLeastVersion(version: string, minimum: string): boolean {
  const have = version.split('.').map(Number);
  const need = minimum.split('.').map(Number);
  for (let index = 0; index < need.length; index += 1) {
    const difference = (have[index] ?? 0) - (need[index] ?? 0);
    if (difference !== 0) return difference > 0;
  }
  return true;
}
