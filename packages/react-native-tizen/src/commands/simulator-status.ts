import type { Reporter } from '../cli/reporter.ts';
import {
  simulatorCommandLine,
  simulatorInstance,
  type SimulatorInstance,
} from '../simulator/instance.ts';
import { instanceState } from '../simulator/lima.ts';
import {
  describeApp,
  describeRuntime,
  readSimulatorStatus,
  type InstanceStatus,
  type RuntimeStatus,
} from '../simulator/simulator-status.ts';
import { viewerUrl } from '../simulator/viewer.ts';

function describeScreen(instance: SimulatorInstance, status: InstanceStatus | undefined): string {
  const on = status !== undefined && status.screen !== null;
  return on ? `:${instance.display}, 1920x1080` : `off, display :${instance.display}`;
}

function describeViewer(instance: SimulatorInstance, status: InstanceStatus | undefined): string {
  if (status !== undefined && status.viewer !== null) return viewerUrl(instance);
  return `off, port ${instance.viewerPort}: ${simulatorCommandLine('view', instance.name)}`;
}

function printInstance(
  reporter: Reporter,
  instance: SimulatorInstance,
  status: InstanceStatus | undefined,
  current: boolean,
): void {
  reporter.line();
  const marker = current ? `  ${reporter.paint('gray', '(this one)')}` : '';
  reporter.field('Instance', `${instance.name}${marker}`);
  reporter.field('App', describeApp(instance.name, status));
  if (status?.staged === true) {
    reporter.field('Runtime', describeRuntime(status.runtime, status.runtimeBuilt));
  }
  reporter.field('Screen', describeScreen(instance, status));
  reporter.field('Live screen', describeViewer(instance, status));
  reporter.field('Log port', `${instance.logPort} on this Mac`);
}

function describeRuntimes(runtimes: readonly RuntimeStatus[]): string {
  if (runtimes.length === 0) return 'none built: rn-tizen simulator build';
  const keys = runtimes.map((runtime) => describeRuntime(runtime.key, runtime.built));
  return keys.join(', ');
}

/** `rn-tizen simulator status`: the VM, its runtime builds, then `current` and every other instance. */
export async function printSimulatorStatus(
  reporter: Reporter,
  current: SimulatorInstance,
): Promise<void> {
  const state = await instanceState();
  const vm = state === null ? 'not created: rn-tizen simulator run' : state.status.toLowerCase();
  reporter.field('VM', vm);
  if (state?.status !== 'Running') return;
  const status = await readSimulatorStatus(reporter.task());
  reporter.field('Runtimes', describeRuntimes(status.runtimes));
  const own = status.instances.find((entry) => entry.name === current.name);
  printInstance(reporter, current, own, true);
  for (const other of status.instances) {
    if (other !== own) printInstance(reporter, simulatorInstance(other.name), other, false);
  }
}
