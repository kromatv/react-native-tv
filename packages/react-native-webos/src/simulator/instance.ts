import { z } from 'zod';
import { UsageError } from '../cli/command.ts';

/** The instance an action drives when neither `--instance` nor the environment names one. */
export const DEFAULT_INSTANCE = 'default';

/** The environment variable that names the instance, so an agent sets it once. */
export const INSTANCE_ENV = 'RN_TIZEN_SIMULATOR_INSTANCE';

/** What a name may hold: it becomes a directory in the VM. */
export const INSTANCE_NAME = /^[a-z0-9][a-z0-9_-]{0,31}$/;

const SLOT_COUNT = 1000;
const FIRST_DISPLAY = 100;
const FIRST_VIEWER_PORT = 40000;
const FIRST_LOG_PORT = 41000;
const FIRST_VNC_PORT = 42000;

const nameSchema = z.string().regex(INSTANCE_NAME);

/**
 * One app on its own screen in the shared VM. Every number comes from the
 * name, so each agent finds the same ports again without asking the VM.
 */
export interface SimulatorInstance {
  readonly name: string;
  readonly display: number;
  readonly viewerPort: number;
  readonly vncPort: number;
  readonly logPort: number;
}

function fnv1a(text: string): number {
  let hash = 0x811c9dc5;
  for (const byte of new TextEncoder().encode(text)) {
    hash = Math.imul(hash ^ byte, 0x01000193) >>> 0;
  }
  return hash;
}

function slotOf(name: string): number {
  if (name === DEFAULT_INSTANCE) return 0;
  return 1 + (fnv1a(name) % (SLOT_COUNT - 1));
}

/** The instance called `name`, with the display and ports it owns. */
export function simulatorInstance(name: string): SimulatorInstance {
  const slot = slotOf(name);
  return {
    name,
    display: FIRST_DISPLAY + slot,
    viewerPort: FIRST_VIEWER_PORT + slot,
    vncPort: FIRST_VNC_PORT + slot,
    logPort: FIRST_LOG_PORT + slot,
  };
}

function requestedName(flag: string | undefined, env: NodeJS.ProcessEnv): [string, string] {
  if (flag !== undefined) return [flag, '--instance'];
  const fromEnv = env[INSTANCE_ENV];
  if (fromEnv !== undefined && fromEnv !== '') return [fromEnv, INSTANCE_ENV];
  return [DEFAULT_INSTANCE, INSTANCE_ENV];
}

/** The instance `--instance` names, else the environment's, else {@link DEFAULT_INSTANCE}. */
export function chooseInstance(
  flag: string | undefined,
  env: NodeJS.ProcessEnv,
): SimulatorInstance {
  const [name, source] = requestedName(flag, env);
  if (!nameSchema.safeParse(name).success) {
    throw new UsageError(
      `${source} must be 1 to 32 lowercase letters, digits, - or _, starting with a letter or digit: got ${name}`,
      'simulator',
    );
  }
  return simulatorInstance(name);
}

/** `rn-tizen simulator <action>` as the user types it for `instance`. */
export function simulatorCommandLine(action: string, instance: string): string {
  const flag = instance === DEFAULT_INSTANCE ? '' : ` --instance ${instance}`;
  return `rn-tizen simulator ${action}${flag}`;
}

/** What the guest scripts read to act on `instance`. */
export function instanceEnv(instance: SimulatorInstance): Readonly<Record<string, string>> {
  return {
    [INSTANCE_ENV]: instance.name,
    RN_TIZEN_SIMULATOR_DISPLAY: String(instance.display),
    RN_TIZEN_SIMULATOR_VIEWER_PORT: String(instance.viewerPort),
    RN_TIZEN_SIMULATOR_VNC_PORT: String(instance.vncPort),
  };
}
