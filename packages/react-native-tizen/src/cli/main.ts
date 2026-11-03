#!/usr/bin/env node
import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { z } from 'zod';
import { buildRuntimeCommand } from '../commands/build-runtime.ts';
import { bundleCommand } from '../commands/bundle.ts';
import { devicesCommand } from '../commands/devices.ts';
import { doctorCommand } from '../commands/doctor.ts';
import { keyCommand } from '../commands/key.ts';
import { logsCommand } from '../commands/logs.ts';
import { packageCommand } from '../commands/package.ts';
import { perfCommand } from '../commands/perf.ts';
import { runCommand } from '../commands/run.ts';
import { screenshotCommand } from '../commands/screenshot.ts';
import { setupCommand } from '../commands/setup.ts';
import { simulatorCommand } from '../commands/simulator.ts';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { colorsEnabled, painter } from '../paint.ts';
import { UsageError, type Command } from './command.ts';
import { describeError } from './describe-error.ts';
import { commandHelp, programHelp } from './help.ts';
import { closestMatch } from './suggest.ts';

const COMMANDS: readonly Command[] = [
  doctorCommand,
  devicesCommand,
  setupCommand,
  buildRuntimeCommand,
  bundleCommand,
  packageCommand,
  runCommand,
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

async function main(argv: readonly string[]): Promise<void> {
  const paint = painter(colorsEnabled(process.stdout));
  const [name, ...args] = argv;
  if (name === undefined || HELP_FLAGS.has(name) || name === 'help') {
    const topic = name === 'help' ? args[0] : undefined;
    const help =
      topic === undefined
        ? programHelp(COMMANDS, version(), paint)
        : commandHelp(findCommand(topic), paint);
    process.stdout.write(`${help}\n`);
    return;
  }
  if (name === '--version' || name === '-V') {
    process.stdout.write(`${version()}\n`);
    return;
  }
  const command = findCommand(name);
  if (args.some((arg) => HELP_FLAGS.has(arg))) {
    process.stdout.write(`${commandHelp(command, paint)}\n`);
    return;
  }
  await command.run(args, { cwd: process.cwd() });
}

try {
  await main(process.argv.slice(2));
} catch (error) {
  const paint = painter(colorsEnabled(process.stderr));
  const report = describeError(error);
  const lines = [`${paint('red', '✗')} ${paint('bold', report.message)}`];
  for (const detail of report.details) lines.push(paint('gray', `  ${detail}`));
  lines.push(`  ${paint('cyan', report.hint)}`);
  process.stderr.write(`${lines.join('\n')}\n`);
  process.exitCode = report.exitCode;
}
