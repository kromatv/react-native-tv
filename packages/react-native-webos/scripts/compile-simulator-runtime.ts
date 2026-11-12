import { resolve } from 'node:path';
import { loadAppProject } from '../src/app/app-project.ts';
import { Reporter } from '../src/cli/reporter.ts';
import { buildSimulatorRuntime } from '../src/simulator/simulator-runtime.ts';
import type { Task } from '../src/task.ts';

function runHere(
  task: Task,
  command: readonly string[],
  env: Readonly<Record<string, string>> = {},
  onLine?: (line: string) => void,
): Promise<string> {
  return task.run(command, { env: { ...process.env, ...env }, onLine });
}

const app = loadAppProject(resolve(process.argv[2] ?? 'example'));
const reporter = new Reporter({ verbose: true });
await reporter.step(
  'Simulator runtime',
  (task) => buildSimulatorRuntime(task, app, () => undefined, runHere),
  (key) => `aarch64 Linux, ${key}`,
);
