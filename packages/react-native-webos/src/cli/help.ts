import type { Paint } from '../paint.ts';
import type { Command, OptionSpec } from './command.ts';

const PROGRAM = 'rn-tizen';

function optionLabel(name: string, spec: OptionSpec): string {
  const short = spec.short === undefined ? '' : `, -${spec.short}`;
  const value = spec.type === 'string' ? ` ${spec.placeholder ?? '<value>'}` : '';
  return `--${name}${short}${value}`;
}

function table(rows: readonly (readonly [string, string])[], paint: Paint): string[] {
  const width = Math.max(...rows.map(([label]) => label.length)) + 3;
  return rows.map(([label, text]) => `  ${paint('bold', label.padEnd(width))}${text}`);
}

/** The help `rn-tizen` prints with no command, or with `--help`. */
export function programHelp(commands: readonly Command[], version: string, paint: Paint): string {
  return [
    `${paint('bold', PROGRAM)} ${version}: build an Expo app into a Samsung TV package, and run it`,
    '',
    `Usage: ${PROGRAM} <command> [options]`,
    '',
    'Commands',
    ...table(
      commands.map((command) => [command.name, command.summary]),
      paint,
    ),
    '',
    `Run ${PROGRAM} <command> --help for a command's options.`,
  ].join('\n');
}

/** The help `rn-tizen <command> --help` prints. */
export function commandHelp(command: Command, paint: Paint): string {
  const options = Object.entries(command.options).map(
    ([name, spec]) => [optionLabel(name, spec), spec.description] as const,
  );
  const lines = [`Usage: ${PROGRAM} ${command.name} [options]`, '', command.summary];
  if (options.length > 0) lines.push('', 'Options', ...table(options, paint));
  if (command.examples.length > 0) {
    lines.push('', 'Examples', ...command.examples.map((example) => `  ${example}`));
  }
  return lines.join('\n');
}
