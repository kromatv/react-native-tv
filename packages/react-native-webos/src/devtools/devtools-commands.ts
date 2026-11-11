import { z } from 'zod';
import { TIZEN_PLUGIN } from '../config/tizen-settings.ts';
import { readKnownDevices } from '../tizen/known-devices.ts';

/** A dev tools command turned into the `rn-tizen` arguments its job runs. */
export interface JobPlan {
  readonly kind: string;
  readonly title: string;
  readonly args: readonly string[];
  /** Only one job of this kind runs at a time: a new one stops the last. */
  readonly exclusive: boolean;
  /** The job bundles the app, so the app's Metro config must know the `tizen` platform. */
  readonly bundles: boolean;
}

const emptyArgs = z.object({}).strict();
const deviceArgs = z.object({ device: z.string().max(64) }).strict();
const nameArgs = z
  .object({ name: z.string().regex(/^[A-Za-z0-9_.-]{1,64}$/, 'letters, digits, . _ -') })
  .strict();

/** A command's arguments that cannot run, told to the user as they are. */
export class CommandInputError extends Error {}

/** The page of the dev tools plugin inside the dev server, opened on one job. */
export function jobPageUrl(devServer: string, id: string): string {
  const url = new URL(`/_expo/plugins/${TIZEN_PLUGIN}/`, devServer);
  url.searchParams.set('job', id);
  return url.toString();
}

async function lastDevice(): Promise<string> {
  const [last] = await readKnownDevices();
  if (last === undefined)
    throw new CommandInputError('No TV was used before: enter its IP address.');
  return `${last.host}:${last.port}`;
}

/** What each command of `expo-module.config.json` runs, given Expo's arguments and its dev server. */
export async function planCommand(
  command: string,
  args: unknown,
  devServer: string,
): Promise<JobPlan> {
  if (command === 'run-tv') {
    const { device } = deviceArgs.parse(args);
    const target = device.trim() === '' ? await lastDevice() : device.trim();
    return {
      kind: command,
      title: `Run on TV ${target}`,
      args: ['run', '--device', target, '--dev-server', devServer],
      exclusive: true,
      bundles: true,
    };
  }
  if (command === 'run-simulator') {
    emptyArgs.parse(args);
    return {
      kind: command,
      title: 'Run in simulator',
      args: ['simulator', 'run', '--dev-server', devServer],
      exclusive: true,
      bundles: true,
    };
  }
  if (command === 'find-tvs') {
    emptyArgs.parse(args);
    return {
      kind: command,
      title: 'Find TVs',
      args: ['devices'],
      exclusive: false,
      bundles: false,
    };
  }
  if (command === 'create-certificate') {
    const { name } = nameArgs.parse(args);
    return {
      kind: command,
      title: `Create certificate ${name}`,
      args: ['certificate', 'create-author', name],
      exclusive: false,
      bundles: false,
    };
  }
  throw new CommandInputError(`Unknown command ${command}`);
}

/**
 * What `t` in `expo start` runs: Run on TV or Run in simulator, ending once the
 * app is launched, since its logs reach Expo's terminal through the dev server.
 */
export function openPlan(target: string, devServer: string): JobPlan {
  const plan =
    target === 'simulator'
      ? { kind: 'run-simulator', title: 'Run in simulator', args: ['simulator', 'run'] }
      : { kind: 'run-tv', title: `Run on TV ${target}`, args: ['run', '--device', target] };
  return {
    ...plan,
    args: [...plan.args, '--dev-server', devServer, '--no-logs'],
    exclusive: true,
    bundles: true,
  };
}
