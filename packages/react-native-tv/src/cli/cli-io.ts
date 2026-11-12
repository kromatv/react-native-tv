import type { RunScript } from '../expo-cli/run-platform-cli.ts';
import type { RunManager } from '../expo-patch/run-manager.ts';

/** Where `rn-tv` reads the app from and prints, and what it runs, replaced in tests. */
export interface CliIo {
  readonly cwd: string;
  /** The environment, which says when a package manager runs the command as an install script. */
  readonly env?: NodeJS.ProcessEnv;
  out(text: string): void;
  err(text: string): void;
  readonly runScript?: RunScript;
  readonly runManager?: RunManager;
}
