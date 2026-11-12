import { spawnSync } from 'node:child_process';
import { constants } from 'node:os';

/** How a Node script ended. */
export interface ScriptResult {
  readonly status: number | null;
  readonly signal: NodeJS.Signals | null;
  readonly error?: Error;
}

/** Runs a Node script with the terminal of this process, and waits for it. */
export type RunScript = (script: string, args: readonly string[]) => ScriptResult;

const runWithNode: RunScript = (script, args) =>
  spawnSync(process.execPath, [script, ...args], { stdio: 'inherit' });

/**
 * Runs `run` of a TV platform's command with `argv`, and gives the exit code
 * a shell would: the command's own, or 128 plus the signal that ended it.
 */
export function runPlatformCli(
  script: string,
  argv: readonly string[],
  run: RunScript = runWithNode,
): number {
  const result = run(script, ['run', ...argv]);
  if (result.error !== undefined) throw result.error;
  if (result.status !== null) return result.status;
  return 128 + (result.signal === null ? 0 : constants.signals[result.signal]);
}
