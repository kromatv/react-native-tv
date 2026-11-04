import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { z } from 'zod';
import { buildRuntimeCommand } from '../commands/build-runtime.ts';
import { bundleCommand } from '../commands/bundle.ts';
import { certificateCommand } from '../commands/certificate.ts';
import { devicesCommand } from '../commands/devices.ts';
import { doctorCommand } from '../commands/doctor.ts';
import { initCommand } from '../commands/init.ts';
import { installCommand } from '../commands/install.ts';
import { keyCommand } from '../commands/key.ts';
import { logsCommand } from '../commands/logs.ts';
import { packageCommand } from '../commands/package.ts';
import { perfCommand } from '../commands/perf.ts';
import { runCommand } from '../commands/run.ts';
import { screenshotCommand } from '../commands/screenshot.ts';
import { setupCommand } from '../commands/setup.ts';
import { simulatorCommand } from '../commands/simulator.ts';
import { uninstallCommand } from '../commands/uninstall.ts';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { colorsEnabled, painter, type TextOutput } from '../paint.ts';
import { UsageError, type Command } from './command.ts';
import { describeError } from './describe-error.ts';
import { commandHelp, programHelp } from './help.ts';
import { closestMatch } from './suggest.ts';

const COMMANDS: readonly Command[] = [
  initCommand,
  doctorCommand,
  devicesCommand,
  certificateCommand,
  setupCommand,
  buildRuntimeCommand,
  bundleCommand,
  packageCommand,
  runCommand,
  installCommand,
  uninstallCommand,
  logsCommand,
  keyCommand,
  screenshotCommand,
  perfCommand,
  simulatorCommand,
];

const HELP_FLAGS = new Set(['--help', '-h']);

function version(): string {
  const manifest = readFileSync(join(PACKAGE_ROOT, 'package.json'), 'utf8');
  return z.object({ version: z.string() }).parse(JSON.parse(manifest)).version;
}

function findCommand(name: string): Command {
  const command = COMMANDS.find((candidate) => candidate.name === name);
  if (command !== undefined) return command;
  const suggestion = closestMatch(
    name,
    COMMANDS.map((candidate) => candidate.name),
  );
  const hint = suggestion === null ? '' : `. Did you mean ${suggestion}?`;
  throw new UsageError(`Unknown command ${name}${hint}`, null);
}

/** Where {@link runCli} prints. */
export interface CliStreams {
  readonly out: TextOutput;
  readonly err: TextOutput;
}

async function dispatch(argv: readonly string[], out: TextOutput): Promise<void> {
  const paint = painter(colorsEnabled(out));
  const [name, ...args] = argv;
  if (name === undefined || HELP_FLAGS.has(name) || name === 'help') {
    const topic = name === 'help' ? args[0] : undefined;
    const help =
      topic === undefined
        ? programHelp(COMMANDS, version(), paint)
        : commandHelp(findCommand(topic), paint);
    out.write(`${help}\n`);
    return;
  }
  if (name === '--version' || name === '-V') {
    out.write(`${version()}\n`);
    return;
  }
  const command = findCommand(name);
  if (args.some((arg) => HELP_FLAGS.has(arg))) {
    out.write(`${commandHelp(command, paint)}\n`);
    return;
  }
  await command.run(args, { cwd: process.cwd() });
}

function report(error: unknown, err: TextOutput): number {
  const paint = painter(colorsEnabled(err));
  const described = describeError(error);
  const lines = [`${paint('red', '✗')} ${paint('bold', described.message)}`];
  for (const detail of described.details) lines.push(paint('gray', `  ${detail}`));
  lines.push(`  ${paint('cyan', described.hint)}`);
  err.write(`${lines.join('\n')}\n`);
  return described.exitCode;
}

/** Runs `rn-tizen` with `argv` and returns its exit code; a failure is printed to `err`. */
export async function runCli(argv: readonly string[], streams: CliStreams): Promise<number> {
  try {
    await dispatch(argv, streams.out);
    return 0;
  } catch (error) {
    return report(error, streams.err);
  }
}
