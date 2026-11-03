import { spawn, type StdioOptions } from 'node:child_process';
import type { Readable } from 'node:stream';
import { z } from 'zod';
import { findProgram } from '../doctor/find-program.ts';
import { FixableError } from '../errors.ts';
import { CommandError } from '../exec.ts';
import type { Task } from '../task.ts';

export const LIMA_INSTANCE = 'rn-tizen-sim';

/** The directories the VM mounts, each at the same path as on the Mac. */
export interface MountParams {
  readonly ProjectDir: string;
  readonly CacheDir: string;
}

const instanceSchema = z.object({
  status: z.string(),
  param: z.record(z.string(), z.string()).optional(),
  config: z.object({ mounts: z.array(z.object({ location: z.string() })).optional() }).optional(),
});

/** What `limactl list` reports about the simulator's VM. */
export type InstanceState = z.infer<typeof instanceSchema>;

/** Where a guest command's standard streams come from and go to, and its environment. */
export interface GuestStreams {
  readonly stdin?: Readable;
  readonly stdout?: number | 'inherit';
  readonly env?: Readonly<Record<string, string>>;
}

const HOMEBREW_BIN = '/opt/homebrew/bin';

function limactl(): string {
  const path = findProgram('limactl', [HOMEBREW_BIN]);
  if (path === null) throw new FixableError('Lima is not installed', 'brew install lima');
  return path;
}

/** Fails with what to install unless this Mac can run the simulator's VM. */
export function requireLima(): void {
  if (process.platform !== 'darwin' || process.arch !== 'arm64') {
    throw new FixableError(
      'The Tizen simulator runs on Apple silicon Macs only',
      'Use a Mac with an M-series chip, or a real TV (rn-tizen run).',
    );
  }
  limactl();
}

function guestCommand(command: readonly string[], env: Readonly<Record<string, string>>): string[] {
  const assignments = Object.entries(env).map(([name, value]) => `${name}=${value}`);
  return [limactl(), 'shell', '--workdir', '/', LIMA_INSTANCE, 'env', ...assignments, ...command];
}

/** Runs `command` in the VM as a task step: quiet unless `--verbose`, its output in any error. */
export function guestRun(
  task: Task,
  command: readonly string[],
  env: Readonly<Record<string, string>> = {},
  onLine?: (line: string) => void,
): Promise<string> {
  return task.run(guestCommand(command, env), { onLine });
}

/** Runs `command` in the VM with its streams wired to the Mac's as given; stderr is inherited. */
export function guestSpawn(command: readonly string[], streams: GuestStreams = {}): Promise<void> {
  const shell = guestCommand(command, streams.env ?? {});
  const stdio: StdioOptions = [
    streams.stdin ? 'pipe' : 'ignore',
    streams.stdout ?? 'inherit',
    'inherit',
  ];
  return new Promise((resolve, reject) => {
    const [program = '', ...args] = shell;
    const child = spawn(program, args, { stdio });
    if (streams.stdin && child.stdin) streams.stdin.pipe(child.stdin);
    child.on('error', reject);
    child.on('close', (exitCode) => {
      if (exitCode === 0) resolve();
      else reject(new CommandError(shell, exitCode, ''));
    });
  });
}

function limactlStdout(args: readonly string[]): Promise<string> {
  return new Promise((resolve, reject) => {
    const child = spawn(limactl(), args, { stdio: ['ignore', 'pipe', 'ignore'] });
    const chunks: Buffer[] = [];
    child.stdout.on('data', (chunk: Buffer) => chunks.push(chunk));
    child.on('error', reject);
    child.on('close', () => resolve(Buffer.concat(chunks).toString('utf8')));
  });
}

/** The VM's state, or null when it was never created. */
export async function instanceState(): Promise<InstanceState | null> {
  const output = (await limactlStdout(['list', '--format', 'json', LIMA_INSTANCE])).trim();
  return output === '' ? null : instanceSchema.parse(JSON.parse(output));
}

function paramExpression(params: MountParams): string {
  return Object.entries(params)
    .map(([name, value]) => `.param.${name} = ${JSON.stringify(value)}`)
    .join(' | ');
}

/** Creates the VM from `template` with its mounts set to `params`. */
export async function createInstance(
  task: Task,
  template: string,
  params: MountParams,
): Promise<void> {
  const name = `--name=${LIMA_INSTANCE}`;
  await task.run([
    limactl(),
    'create',
    name,
    '--tty=false',
    '--set',
    paramExpression(params),
    template,
  ]);
}

/** Points a stopped VM's mounts at `params`. */
export async function editInstance(task: Task, params: MountParams): Promise<void> {
  await task.run([
    limactl(),
    'edit',
    '--tty=false',
    '--set',
    paramExpression(params),
    LIMA_INSTANCE,
  ]);
}

/** Adds a read-only mount of `location` to a stopped VM, at the same path, beside its others. */
export async function addMount(task: Task, location: string): Promise<void> {
  const mount = JSON.stringify({ location, writable: false });
  await task.run([
    limactl(),
    'edit',
    '--tty=false',
    '--set',
    `.mounts += [${mount}]`,
    LIMA_INSTANCE,
  ]);
}

export async function startInstance(task: Task): Promise<void> {
  await task.run([limactl(), 'start', '--tty=false', LIMA_INSTANCE]);
}

export async function stopInstance(task: Task): Promise<void> {
  await task.run([limactl(), 'stop', LIMA_INSTANCE]);
}

export async function deleteInstance(task: Task): Promise<void> {
  await task.run([limactl(), 'delete', '--force', LIMA_INSTANCE]);
}
