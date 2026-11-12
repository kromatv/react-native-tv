import { runnablePlatforms } from '../expo-cli/extend-expo-cli.ts';
import { runPlatformCli } from '../expo-cli/run-platform-cli.ts';
import type { CliIo } from './cli-io.ts';

/** `rn-tv run:<platform>`: the platform's own `run`, with the arguments given. */
export function runCommand(platform: string, argv: readonly string[], io: CliIo): number {
  const scripts = runnablePlatforms(io.cwd);
  const script = scripts.get(platform);
  if (script !== undefined) return runPlatformCli(script, argv, io.runScript);
  const known = [...scripts.keys()].map((name) => `run:${name}`).join(', ');
  io.err(
    known === ''
      ? `This app depends on no TV platform package that runs ${platform}.\n`
      : `This app has no platform ${platform}. It has ${known}.\n`,
  );
  return 2;
}
