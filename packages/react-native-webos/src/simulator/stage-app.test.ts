import { EventEmitter } from 'node:events';
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { PassThrough } from 'node:stream';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { CommandError } from '../exec.ts';
import { simulatorInstance } from './instance.ts';
import { guestSpawn } from './lima.ts';
import { stageApp } from './stage-app.ts';

const archived: string[][] = [];
let tarExit = 0;

vi.mock('./lima.ts', () => ({ guestSpawn: vi.fn(async () => undefined) }));
vi.mock('../package/app-config-file.ts', () => ({ writeAppConfig: vi.fn() }));
vi.mock('node:child_process', () => ({
  spawn: vi.fn((program: string, args: string[]) => {
    archived.push([program, ...args]);
    const child = Object.assign(new EventEmitter(), { stdout: new PassThrough() });
    setImmediate(() => {
      child.stdout.end();
      child.emit('close', tarExit);
    });
    return child;
  }),
}));

let root: string;

function file(name: string): string {
  const path = join(root, name);
  writeFileSync(path, name);
  return path;
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-stage-'));
  archived.length = 0;
  tarExit = 0;
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('stageApp', () => {
  it('streams the res directory to the guest stage script for the runtime key', async () => {
    mkdirSync(join(root, 'assets'));
    const instance = simulatorInstance('ts');

    await stageApp(
      instance,
      fakeApp(root),
      { bytecode: file('index.hbc'), assetsDir: join(root, 'assets') },
      { host: '192.168.5.2', port: instance.logPort },
      { key: '0123456789ab', icuData: file('icudt77l.dat') },
    );

    expect(archived[0]?.[0]).toBe('/usr/bin/tar');
    expect(guestSpawn).toHaveBeenCalledWith(
      ['bash', expect.stringMatching(/guest\/stage\.sh$/)],
      expect.objectContaining({
        env: expect.objectContaining({ RN_TIZEN_SIMULATOR_RUNTIME: '0123456789ab' }),
      }),
    );
  });

  it('fails as a command error when tar does', async () => {
    tarExit = 1;

    await expect(
      stageApp(simulatorInstance('ts'), fakeApp(root), { bytecode: file('index.hbc') }, null, {
        key: '',
        icuData: file('icudt77l.dat'),
      }),
    ).rejects.toBeInstanceOf(CommandError);
  });
});
