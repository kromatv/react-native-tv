/** A simulator action's arguments: what it acts on, then its options. */
export interface ActionArgs {
  readonly positionals: readonly string[];
  readonly options: readonly string[];
}

const NEGATIVE_NUMBER = /^-\d/;

/**
 * Splits `key Down Enter --verbose` style arguments at the first option. A
 * negative number, as in `wheel -3`, is what the action acts on, not an option.
 */
export function splitActionArgs(args: readonly string[]): ActionArgs {
  const firstOption = args.findIndex((arg) => arg.startsWith('-') && !NEGATIVE_NUMBER.test(arg));
  const end = firstOption === -1 ? args.length : firstOption;
  return { positionals: args.slice(0, end), options: args.slice(end) };
}
