import { relative, resolve } from 'node:path';
import { loadAppProject, type AppProject } from '../app/app-project.ts';
import { entryPath, type CommandContext } from '../cli/command.ts';
import { chooseLogEndpoint, LOG_OPTIONS } from '../cli/log-options.ts';
import { Reporter } from '../cli/reporter.ts';
import type { LogEndpoint } from '../package/runtime-config.ts';

/** The options of every command that builds a package. */
export const BUILD_OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description:
      "The TV the build is for: a development build streams its logs to this computer's address on that TV's network",
  },
  ...LOG_OPTIONS,
  'dev-server': {
    type: 'string',
    placeholder: '<origin>',
    description:
      'Load the bundle from this `expo start` dev server, such as http://192.168.1.20:8081, instead of packaging it',
  },
} as const;

interface BuildFlags {
  readonly device?: string;
  readonly release?: boolean;
  readonly dev?: boolean;
  readonly 'log-host'?: string;
  readonly 'log-port'?: string;
  readonly 'dev-server'?: string;
  readonly project?: string;
  readonly entry?: string;
  readonly verbose?: boolean;
}

interface BuildBase {
  readonly reporter: Reporter;
  readonly app: AppProject;
}

interface PreparedBuild extends BuildBase {
  readonly log: LogEndpoint | null;
}

/** Reads the app a build is of. */
export function openBuild(flags: BuildFlags, context: CommandContext): BuildBase {
  const reporter = new Reporter({ verbose: flags.verbose ?? false });
  const app = loadAppProject(resolve(context.cwd, flags.project ?? '.'), entryPath(flags, context));
  return { reporter, app };
}

/**
 * Settles where the build streams its logs, on the network of the TV at
 * `deviceHost` when there is one, and prints what was settled.
 */
export function settleBuild(
  command: string,
  flags: BuildFlags,
  base: BuildBase,
  deviceHost: string | null,
  cwd: string,
): PreparedBuild {
  const { reporter, app } = base;
  const log = chooseLogEndpoint({
    command,
    release: flags.release,
    host: flags['log-host'],
    port: flags['log-port'],
    deviceHost,
  });
  const where = relative(cwd, app.root);
  const location = where === '' ? '' : `  ${reporter.paint('gray', where)}`;
  reporter.field('App', `${app.config.packageId} ${app.config.version}${location}`);
  reporter.field(
    'Build',
    log === null ? 'release' : `development, logs to ${log.host}:${log.port}`,
  );
  return { ...base, log };
}
