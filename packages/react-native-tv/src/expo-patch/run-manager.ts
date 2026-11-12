import { execFileSync } from 'node:child_process';
import { z } from 'zod';

export type ManagerCommand = 'bun' | 'pnpm' | 'yarn';

/** Runs a package manager's command in a directory and gives what it printed. */
export type RunManager = (command: ManagerCommand, args: readonly string[], cwd: string) => string;

const MAX_OUTPUT_BYTES = 16 * 1024 * 1024;

const printedSchema = z.object({ stdout: z.string().catch(''), stderr: z.string().catch('') });

/** Runs the manager found on `PATH`, and throws what it printed when it fails. */
export const runManager: RunManager = (command, args, cwd) => {
  try {
    return execFileSync(command, [...args], {
      cwd,
      encoding: 'utf8',
      maxBuffer: MAX_OUTPUT_BYTES,
      stdio: ['ignore', 'pipe', 'pipe'],
    });
  } catch (error) {
    const { stdout, stderr } = printedSchema.parse(Object(error));
    const printed = `${stdout}${stderr}`.trim();
    throw new Error(
      `${command} ${args.join(' ')} failed:\n${printed === '' ? String(error) : printed}`,
      { cause: error },
    );
  }
};
