/** A simulator action's arguments: what it acts on, then its options. */
export interface ActionArgs {
  readonly positionals: readonly string[];
  readonly options: readonly string[];
}

/** Splits `key Down Enter --verbose` style arguments at the first option. */
export function splitActionArgs(args: readonly string[]): ActionArgs {
  const firstOption = args.findIndex((arg) => arg.startsWith('-'));
  const end = firstOption === -1 ? args.length : firstOption;
  return { positionals: args.slice(0, end), options: args.slice(end) };
}
