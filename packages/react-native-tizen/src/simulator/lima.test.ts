import { EventEmitter } from 'node:events';
import { PassThrough } from 'node:stream';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { findProgram } from '../doctor/find-program.ts';
import { CommandError } from '../exec.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import {
  addMount,
  createInstance,
  deleteInstance,
  editInstance,
  guestRun,
  guestSpawn,
  instanceState,
  requireLima,
  startInstance,
  stopInstance,
} from './lima.ts';

const LIMACTL = '/opt/homebrew/bin/limactl';

const spawned: { program: string; args: readonly string[] }[] = [];
let exitCode = 0;
let stdout = '';

vi.mock('../doctor/find-program.ts', () => ({ findProgram: vi.fn() }));
vi.mock('node:child_process', () => ({
  spawn: vi.fn((program: string, args: readonly string[]) => {
    spawned.push({ program, args });
    const child = Object.assign(new EventEmitter(), {
      stdout: new PassThrough(),
      stdin: new PassThrough(),
    });
    setImmediate(() => {
      child.stdout.end(stdout);
      child.emit('close', exitCode);
    });
    return child;
  }),
}));

beforeEach(() => {
  vi.mocked(findProgram).mockReturnValue(LIMACTL);
  spawned.length = 0;
  exitCode = 0;
  stdout = '';
});

afterEach(() => {
  vi.clearAllMocks();
});

describe('requireLima', () => {
  it.runIf(process.platform === 'darwin' && process.arch === 'arm64')(
    'says how to install Lima when limactl is missing',
    () => {
      vi.mocked(findProgram).mockReturnValue(null);

      expect(() => requireLima()).toThrow('Lima is not installed');
    },
  );

  it.runIf(process.platform !== 'darwin' || process.arch !== 'arm64')(
    'refuses a machine other than an Apple silicon Mac',
    () => {
      expect(() => requireLima()).toThrow('The Tizen simulator runs on Apple silicon Macs only');
    },
  );
});

describe('guestRun', () => {
  it('runs the command in the VM with the environment, through the limactl it found', async () => {
    const task = recordingTask();

    await guestRun(task, ['bash', 'run.sh'], { A: '1' });

    expect(task.commands[0]).toEqual([
      LIMACTL,
      'shell',
      '--workdir',
      '/',
      'rn-tizen-sim',
      'env',
      'A=1',
      'bash',
      'run.sh',
    ]);
  });

  it('finds limactl in Homebrew when it is not on PATH', async () => {
    await guestRun(recordingTask(), ['true']);

    expect(findProgram).toHaveBeenCalledWith('limactl', ['/opt/homebrew/bin']);
  });
});

describe('guestSpawn', () => {
  it('resolves when the guest command exits 0', async () => {
    await guestSpawn(['bash', 'logs.sh']);

    expect(spawned[0]?.program).toBe(LIMACTL);
  });

  it('pipes stdin into the guest command', async () => {
    const stdin = new PassThrough();

    await guestSpawn(['bash', 'stage.sh'], { stdin });

    expect(spawned).toHaveLength(1);
  });

  it('fails as a command error when the guest command exits non-zero', async () => {
    exitCode = 3;

    await expect(guestSpawn(['bash', 'screenshot.sh'])).rejects.toBeInstanceOf(CommandError);
  });
});

describe('instanceState', () => {
  it('reads the state limactl list prints', async () => {
    stdout = '{"status":"Running","param":{"CacheDir":"/c"},"config":{"mounts":[]}}\n';

    expect(await instanceState()).toEqual({
      status: 'Running',
      param: { CacheDir: '/c' },
      config: { mounts: [] },
    });
  });

  it('is null for a VM limactl does not know', async () => {
    exitCode = 1;

    expect(await instanceState()).toBeNull();
  });
});

describe('the VM lifecycle', () => {
  it('creates, edits, mounts, starts, stops and deletes the VM by name', async () => {
    const task = recordingTask();
    const params = { ProjectDir: '/p', CacheDir: '/c' };

    await createInstance(task, '/lima.yaml', params);
    await editInstance(task, params);
    await addMount(task, '/q');
    await startInstance(task);
    await stopInstance(task);
    await deleteInstance(task);

    expect(task.commands.map((command) => command[1])).toEqual([
      'create',
      'edit',
      'edit',
      'start',
      'stop',
      'delete',
    ]);
    expect(task.commands[0]).toContain('.param.ProjectDir = "/p" | .param.CacheDir = "/c"');
    expect(task.commands[2]).toContain('.mounts += [{"location":"/q","writable":false}]');
  });
});
