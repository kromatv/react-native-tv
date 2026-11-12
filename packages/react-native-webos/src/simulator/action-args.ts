import type { OptionSpecs } from '../cli/command.ts';

/** A command's arguments: what it acts on, and its options. */
export interface ActionArgs {
  readonly positionals: readonly string[];
  readonly options: readonly string[];
}

const NEGATIVE_NUMBER = /^-\d/;

function takesValue(option: string, specs: OptionSpecs): boolean {
  if (option.includes('=')) return false;
  const name = option.startsWith('--')
    ? option.slice(2)
    : Object.keys(specs).find((key) => specs[key]?.short === option.slice(1));
  return name !== undefined && specs[name]?.type === 'string';
}

/**
 * Splits arguments into what a command acts on and its options, in any order:
 * `key Down Enter --verbose` or `key --instance tv Down`. `specs` says which
 * options take a value. A negative number, as in `wheel -3`, is acted on.
 */
export function splitActionArgs(args: readonly string[], specs: OptionSpecs): ActionArgs {
  const positionals: string[] = [];
  const options: string[] = [];
  for (let index = 0; index < args.length; index += 1) {
    const arg = args[index] ?? '';
    if (!arg.startsWith('-') || NEGATIVE_NUMBER.test(arg)) {
      positionals.push(arg);
      continue;
    }
    options.push(arg);
    const value = args[index + 1];
    if (value !== undefined && takesValue(arg, specs)) {
      options.push(value);
      index += 1;
    }
  }
  return { positionals, options };
}
