import { readFileSync } from 'node:fs';
import { z } from 'zod';
import type { CliIo } from './cli-io.ts';
import { expoPatchCommand } from './expo-patch-command.ts';
import { platformsCommand } from './platforms-command.ts';
import { runCommand } from './run-command.ts';
import { USAGE } from './usage.ts';

const RUN_PREFIX = 'run:';

function version(): string {
  const manifest = readFileSync(new URL('../../package.json', import.meta.url), 'utf8');
  return z.looseObject({ version: z.string() }).parse(JSON.parse(manifest)).version;
}

/** Runs `rn-tv` with `argv`, and gives its exit code. */
export function runCli(argv: readonly string[], io: CliIo): number {
  const [command, ...rest] = argv;
  if (command === undefined || command === '--help' || command === '-h') {
    io.out(USAGE);
    return 0;
  }
  if (command === '--version' || command === '-v') {
    io.out(`${version()}\n`);
    return 0;
  }
  if (command === 'platforms') return platformsCommand(io);
  if (command === 'expo-patch') return expoPatchCommand(io);
  if (command.startsWith(RUN_PREFIX)) return runCommand(command.slice(RUN_PREFIX.length), rest, io);
  const [platform, ...options] = rest;
  if (command === 'run' && platform !== undefined) return runCommand(platform, options, io);
  io.err(`Unknown command: ${argv.join(' ')}\n\n${USAGE}`);
  return 2;
}
