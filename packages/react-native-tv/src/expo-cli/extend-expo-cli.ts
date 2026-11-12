import { z } from 'zod';
import { tvPlatformPackages } from '../platforms/tv-platform-packages.ts';
import { runPlatformCli } from './run-platform-cli.ts';

/** One entry of Expo CLI's command table: it loads the function that runs the command. */
export type ExpoCommandLoader = () => Promise<(argv?: string[]) => unknown>;

/** What {@link extendExpoCli} reaches outside its arguments, replaced in tests. */
export interface ExpoCliHost {
  readonly projectRoot: string;
  /** Runs `run` of the platform command in `script`, and gives its exit code. */
  run(script: string, argv: readonly string[]): number;
  exit(code: number): void;
  warn(message: string): void;
}

const commandTableSchema = z.record(
  z.string(),
  z.custom<ExpoCommandLoader>((value) => typeof value === 'function'),
);

function isCommandTable(value: unknown): value is Record<string, ExpoCommandLoader> {
  return commandTableSchema.safeParse(value).success;
}

function defaultHost(): ExpoCliHost {
  return {
    projectRoot: process.cwd(),
    run: (script, argv) => runPlatformCli(script, argv),
    exit: (code) => process.exit(code),
    warn: (message) => console.warn(message),
  };
}

/** The script of the command of each platform that names both, by platform. */
export function runnablePlatforms(projectRoot: string): Map<string, string> {
  const scripts = new Map<string, string>();
  for (const { platform, cli } of tvPlatformPackages(projectRoot)) {
    if (platform !== null && cli !== null && !scripts.has(platform)) scripts.set(platform, cli);
  }
  return scripts;
}

function addRunCommands(
  commands: Record<string, ExpoCommandLoader>,
  scripts: ReadonlyMap<string, string>,
  host: ExpoCliHost,
): void {
  const runOn = (script: string, argv: readonly string[]) => host.exit(host.run(script, argv));
  for (const [platform, script] of scripts) {
    commands[`run:${platform}`] ??= () => Promise.resolve((argv = []) => runOn(script, argv));
  }
  const expoRun = commands.run;
  if (expoRun === undefined || scripts.size === 0) return;
  commands.run = async () => {
    const run = await expoRun();
    return (argv = []) => {
      const script = scripts.get(argv[0] ?? '');
      return script === undefined ? run(argv) : runOn(script, argv.slice(1));
    };
  };
}

/**
 * Adds the TV platforms of the app in the current directory to Expo CLI's
 * command table: `run:<platform>` for each, and `run <platform>`, which run
 * the platform's own `run` with the arguments given. Expo CLI calls it through
 * the patch `rn-tv expo-patch` applies. A command Expo CLI has already is
 * kept, and a failure here leaves Expo's commands as they were.
 */
export function extendExpoCli(commands: unknown, host: ExpoCliHost = defaultHost()): void {
  try {
    if (!isCommandTable(commands)) throw new Error('Expo CLI gave no command table');
    addRunCommands(commands, runnablePlatforms(host.projectRoot), host);
  } catch (error) {
    const reason = error instanceof Error ? error.message : String(error);
    host.warn(`@kromatv/react-native-tv could not add its commands: ${reason}`);
  }
}
