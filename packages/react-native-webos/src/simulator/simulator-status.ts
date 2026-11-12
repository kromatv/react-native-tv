import { z } from 'zod';
import type { Task } from '../task.ts';
import { INSTANCE_NAME, simulatorCommandLine } from './instance.ts';
import { guestRun } from './lima.ts';
import { RUNTIME_KEY } from './runtime-key.ts';
import { guestScript } from './simulator.ts';

const pid = z.number().int().positive().nullable();
const runtimeKey = z.union([z.string().regex(RUNTIME_KEY), z.literal('')]);

const instanceStatusSchema = z.object({
  name: z.string().regex(INSTANCE_NAME),
  staged: z.boolean(),
  app: pid,
  lastApp: pid,
  crashed: z.boolean(),
  screen: pid,
  viewer: pid,
  runtime: runtimeKey,
  runtimeBuilt: z.boolean(),
});

const runtimeStatusSchema = z.object({ key: runtimeKey, built: z.boolean() });

const statusSchema = z.object({
  runtimes: z.array(runtimeStatusSchema),
  instances: z.array(instanceStatusSchema),
});

/** What runs in one simulator instance, as guest/status.sh reports it. */
export type InstanceStatus = z.infer<typeof instanceStatusSchema>;

/** One runtime build, as guest/status.sh reports it; key `''` is the shared one. */
export type RuntimeStatus = z.infer<typeof runtimeStatusSchema>;

/** Every runtime build and every instance in the VM, as guest/status.sh reports them. */
export type SimulatorStatus = z.infer<typeof statusSchema>;

/** Reads the status line guest/status.sh prints. */
export function parseSimulatorStatus(output: string): SimulatorStatus {
  const line = output.trim().split('\n').at(-1) ?? '';
  return statusSchema.parse(JSON.parse(line));
}

export async function readSimulatorStatus(task: Task): Promise<SimulatorStatus> {
  return parseSimulatorStatus(await guestRun(task, ['bash', guestScript('status')]));
}

/** One line about the app in instance `name`: running, crashed, stopped or never started. */
export function describeApp(name: string, status: InstanceStatus | undefined): string {
  if (status !== undefined && status.app !== null) return `running (pid ${status.app})`;
  if (status?.crashed === true) {
    return `crashed (pid ${status.lastApp}): ${simulatorCommandLine('crash', name)}`;
  }
  if (status?.staged === true) return `stopped: ${simulatorCommandLine('restart', name)}`;
  return `not started: ${simulatorCommandLine('run', name)}`;
}

/**
 * One line about a runtime build: its key, and whether it is built. Key `''`
 * is the shared build every instance ran before each app had its own, which an
 * instance staged then keeps until its next `run`.
 */
export function describeRuntime(key: string, built: boolean): string {
  const name = key === '' ? 'shared' : key;
  return built ? name : `${name}, not built`;
}
