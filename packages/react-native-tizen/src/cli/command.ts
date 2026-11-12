import { resolve } from 'node:path';
import { parseArgs, type ParseArgsOptionDescriptor } from 'node:util';
import { closestMatch } from './suggest.ts';

/** A command-line option, with what `--help` says about it. */
export interface OptionSpec extends ParseArgsOptionDescriptor {
  readonly description: string;
  readonly placeholder?: string;
}

/** A command's options by long name. */
export type OptionSpecs = Readonly<Record<string, OptionSpec>>;

/** Options every command takes. */
export const COMMON_OPTIONS = {
  verbose: {
    type: 'boolean',
    short: 'v',
    description: 'Print everything the underlying tools print',
  },
} as const satisfies OptionSpecs;

/** The option every command that reads an app takes. */
export const PROJECT_OPTIONS = {
  project: {
    type: 'string',
    placeholder: '<dir>',
    description: 'The app directory (default: the current directory)',
  },
} as const satisfies OptionSpecs;

/** The option every command that bundles an app takes. */
export const ENTRY_OPTIONS = {
  entry: {
    type: 'string',
    placeholder: '<file>',
    description: "The JavaScript entry file to bundle (default: the app's main)",
  },
} as const satisfies OptionSpecs;

/** A mistake in how a command was invoked; the CLI prints it with the command's usage. */
export class UsageError extends Error {
  readonly command: string | null;

  constructor(message: string, command: string | null) {
    super(message);
    this.name = 'UsageError';
    this.command = command;
  }
}

/** What a command runs with. */
export interface CommandContext {
  readonly cwd: string;
}

/** The `--entry` file as an absolute path, resolved against the working directory. */
export function entryPath(
  flags: { readonly entry?: string },
  context: CommandContext,
): string | undefined {
  return flags.entry === undefined ? undefined : resolve(context.cwd, flags.entry);
}

/** One `rn-tizen` subcommand. */
export interface Command {
  readonly name: string;
  readonly summary: string;
  readonly options: OptionSpecs;
  readonly examples: readonly string[];
  run(args: readonly string[], context: CommandContext): Promise<void>;
}

function unknownOption(message: string, options: OptionSpecs, command: string): UsageError {
  const typed = /'(-{1,2}[^']+)'/.exec(message)?.[1] ?? '';
  const known = Object.keys(options).map((name) => `--${name}`);
  const suggestion = closestMatch(typed.split('=')[0] ?? typed, known);
  const hint = suggestion === null ? '' : `. Did you mean ${suggestion}?`;
  return new UsageError(`Unknown option ${typed}${hint}`, command);
}

/** Parses a command's arguments strictly, turning Node's parse errors into {@link UsageError}s. */
export function parseCommandArgs<const Options extends OptionSpecs>(
  command: string,
  args: readonly string[],
  options: Options,
) {
  try {
    return parseArgs({ args: [...args], options, strict: true, allowPositionals: false }).values;
  } catch (error) {
    if (!(error instanceof Error)) throw error;
    const code = 'code' in error ? error.code : undefined;
    if (code === 'ERR_PARSE_ARGS_UNKNOWN_OPTION')
      throw unknownOption(error.message, options, command);
    if (typeof code === 'string' && code.startsWith('ERR_PARSE_ARGS')) {
      throw new UsageError(error.message, command);
    }
    throw error;
  }
}
