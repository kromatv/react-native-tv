import { closeSync, mkdirSync, openSync, realpathSync, renameSync } from 'node:fs';
import { homedir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import type { AppProject } from '../app/app-project.ts';
import { UsageError } from '../cli/command.ts';
import { FixableError } from '../errors.ts';
import { cacheDir } from '../cache-dir.ts';
import { findNativeLibraries } from '../libraries/find-native-libraries.ts';
import { PACKAGE_ROOT } from '../package-paths.ts';
import type { Task } from '../task.ts';
import {
  addMount,
  createInstance,
  editInstance,
  guestRun,
  guestSpawn,
  instanceState,
  type InstanceState,
  type MountParams,
  startInstance,
  stopInstance,
} from './lima.ts';
import { instanceEnv, type SimulatorInstance } from './instance.ts';
import { isWithin, mountRoot, requireNarrowMount } from './mount-root.ts';

/** The VM definition, guest scripts, shims, launcher and viewer. */
export const SIMULATOR_DIR = fileURLToPath(new URL('../../simulator', import.meta.url));

/** What {@link ensureVm} had to do. */
export type VmPreparation = 'created' | 'started' | 'running';

/** A lifecycle event the launcher delivers to the app, as a TV would. */
export type LifecycleCommand = 'pause' | 'resume' | 'exit';

export function guestScript(name: string): string {
  return join(SIMULATOR_DIR, 'guest', `${name}.sh`);
}

/** Runs guest/<name>.sh on `instance` as a task step, and returns what it printed. */
export function runInstanceScript(
  task: Task,
  instance: SimulatorInstance,
  name: string,
  args: readonly string[] = [],
): Promise<string> {
  return guestRun(task, ['bash', guestScript(name), ...args], instanceEnv(instance));
}

function simulatorMounts(app: AppProject): MountParams {
  const { reactNative, expoModulesCore, expoModulesJsi } = app.packages;
  const libraries = findNativeLibraries(app.root, [reactNative.dir]).libraries;
  const dirs = [
    PACKAGE_ROOT,
    reactNative.dir,
    expoModulesCore.dir,
    expoModulesJsi.dir,
    ...libraries.map((library) => library.dir),
  ];
  const root = mountRoot(dirs.map((dir) => realpathSync(dir)));
  requireNarrowMount(root, realpathSync(homedir()));
  return { ProjectDir: root, CacheDir: cacheDir() };
}

/** Installs the VM's packages. Repeating it only installs what is missing. */
export async function provisionVm(task: Task): Promise<void> {
  task.progress('packages');
  await guestRun(task, ['sudo', 'bash', guestScript('provision')]);
}

function mountsProject(state: InstanceState, projectDir: string): boolean {
  const mounts = state.config?.mounts ?? [];
  return mounts.some((mount) => isWithin(projectDir, mount.location));
}

async function remount(task: Task, state: InstanceState, params: MountParams): Promise<void> {
  if (state.status === 'Running') await stopInstance(task);
  if (state.param?.CacheDir !== params.CacheDir) {
    const projectDir = state.param?.ProjectDir ?? params.ProjectDir;
    await editInstance(task, { ProjectDir: projectDir, CacheDir: params.CacheDir });
  }
  if (!mountsProject(state, params.ProjectDir)) await addMount(task, params.ProjectDir);
}

/**
 * Makes the VM exist, mount what `app` builds from, run, and hold the packages
 * guest/provision.sh lists. Creating it downloads Ubuntu and installs them,
 * which takes a few minutes once. A project no mount holds yet, such as
 * another checkout, gets a mount of its own beside the others, so the projects
 * mounted before keep theirs; adding it restarts the VM once.
 */
export async function ensureVm(task: Task, app: AppProject): Promise<VmPreparation> {
  const params = simulatorMounts(app);
  mkdirSync(params.CacheDir, { recursive: true });
  const state = await instanceState();
  if (state === null) {
    task.progress('creating');
    await createInstance(task, join(SIMULATOR_DIR, 'lima.yaml'), params);
    task.progress('booting');
    await startInstance(task);
    await provisionVm(task);
    return 'created';
  }
  const mounted =
    mountsProject(state, params.ProjectDir) && state.param?.CacheDir === params.CacheDir;
  const running = mounted && state.status === 'Running';
  if (!running) {
    if (!mounted) await remount(task, state, params);
    task.progress('booting');
    await startInstance(task);
  }
  await provisionVm(task);
  return running ? 'running' : 'started';
}

/** Makes sure the VM runs, for a command that needs no app. */
export async function requireRunningVm(task: Task): Promise<void> {
  const state = await instanceState();
  if (state === null) {
    throw new FixableError(
      'The simulator VM does not exist yet',
      'Run `rn-tizen simulator run` in your app: it creates the VM.',
    );
  }
  if (state.status !== 'Running') await startInstance(task);
}

/**
 * (Re)starts the app staged in `instance` and returns its process id once its
 * window is up. No other instance is touched.
 */
export async function startApp(task: Task, instance: SimulatorInstance): Promise<number> {
  const output = await runInstanceScript(task, instance, 'run');
  return Number(output.trim().split('\n').at(-1));
}

/** The Mac's address as the VM reaches it, for the app's network log sink. */
export async function hostAddress(task: Task): Promise<string> {
  const output = await guestRun(task, ['getent', 'ahostsv4', 'host.lima.internal']);
  const address = output.trim().split(/\s+/)[0];
  if (!address) {
    throw new FixableError(
      'The simulator VM cannot resolve host.lima.internal, the address of this Mac',
      'Check the VM network: `limactl shell rn-tizen-sim getent ahostsv4 host.lima.internal` should print an address.',
    );
  }
  return address;
}

/** Saves the screen of `instance` as a PNG at `path` on the Mac and returns its absolute path. */
export async function saveScreenshot(instance: SimulatorInstance, path: string): Promise<string> {
  const target = resolve(path);
  mkdirSync(dirname(target), { recursive: true });
  const partial = `${target}.partial`;
  const fd = openSync(partial, 'w');
  try {
    await guestSpawn(['bash', guestScript('screenshot')], {
      stdout: fd,
      env: instanceEnv(instance),
    });
  } finally {
    closeSync(fd);
  }
  renameSync(partial, target);
  return target;
}

export async function pressKeys(
  task: Task,
  instance: SimulatorInstance,
  keys: readonly string[],
): Promise<void> {
  await runInstanceScript(task, instance, 'key', keys);
}

export async function sendLifecycle(
  task: Task,
  instance: SimulatorInstance,
  command: LifecycleCommand,
): Promise<void> {
  await runInstanceScript(task, instance, 'control', [command]);
}

export async function openUrl(task: Task, instance: SimulatorInstance, url: string): Promise<void> {
  await sendControl(task, instance, 'open-url', url);
}

/**
 * Hands the app in `instance` what only a TV's own services send: a Smart Hub
 * tile press (`tile <url>`) or a voice command as the shell builds it
 * (`voice <json>`).
 */
export async function sendControl(
  task: Task,
  instance: SimulatorInstance,
  command: 'open-url' | 'tile' | 'voice',
  argument: string,
): Promise<void> {
  if (/[\r\n]/.test(argument)) {
    throw new UsageError(`The ${command} argument cannot hold a line break`, 'simulator');
  }
  await runInstanceScript(task, instance, 'control', [command, argument]);
}

/** Prints the log of the app in `instance` to this terminal, and follows it when asked. */
export function printLog(
  instance: SimulatorInstance,
  options: { readonly lines: number; readonly follow: boolean },
): Promise<void> {
  const follow = options.follow ? ['-f'] : [];
  return guestSpawn(['bash', guestScript('logs'), '-n', String(options.lines), ...follow], {
    env: instanceEnv(instance),
  });
}

/** The symbolized backtrace of every thread at the last crash of the app in `instance`. */
export function crashReport(task: Task, instance: SimulatorInstance): Promise<string> {
  return runInstanceScript(task, instance, 'crash');
}

/** Stops the app, the viewer and the virtual screen of `instance`; the VM and every other instance keep running. */
export async function stopSimulator(task: Task, instance: SimulatorInstance): Promise<void> {
  await runInstanceScript(task, instance, 'stop');
}
