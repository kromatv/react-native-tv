import { spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import { cp, copyFile, mkdtemp, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { basename, join } from 'node:path';
import type { AppProject } from '../app/app-project.ts';
import { BYTECODE_FILE } from '../bundle/bundle-app.ts';
import { CommandError } from '../exec.ts';
import { writeAppConfig } from '../package/app-config-file.ts';
import { RUNTIME_CONFIG_FILE, runtimeConfig, type LogEndpoint } from '../package/runtime-config.ts';
import { instanceEnv, type SimulatorInstance } from './instance.ts';
import { guestSpawn } from './lima.ts';
import { guestScript } from './simulator.ts';

/** What the simulator runs: Hermes bytecode and, from `rn-tizen bundle`, its assets. */
export interface SimulatorBundle {
  readonly bytecode: string;
  readonly assetsDir?: string;
  /** A dev server the runtime loads the bundle from instead. */
  readonly bundleUrl?: string;
}

/** What an instance runs besides the bundle: its runtime build and the ICU data it reads. */
export interface SimulatorRuntime {
  readonly key: string;
  readonly icuData: string;
}

async function writeResources(
  dir: string,
  app: AppProject,
  bundle: SimulatorBundle,
  log: LogEndpoint | null,
  icuData: string,
) {
  await copyFile(bundle.bytecode, join(dir, BYTECODE_FILE));
  await copyFile(icuData, join(dir, basename(icuData)));
  await writeAppConfig(app.root, dir);
  if (bundle.assetsDir !== undefined && existsSync(bundle.assetsDir)) {
    await cp(bundle.assetsDir, join(dir, 'assets'), { recursive: true });
  }
  const config = runtimeConfig(BYTECODE_FILE, log, app.config, bundle.bundleUrl);
  await writeFile(join(dir, RUNTIME_CONFIG_FILE), `${JSON.stringify(config, null, 2)}\n`);
}

function archive(dir: string) {
  const command = ['/usr/bin/tar', '-c', '-h', '--no-mac-metadata', '--no-xattrs', '-C', dir, '.'];
  const [program = '', ...args] = command;
  const tar = spawn(program, args, {
    stdio: ['ignore', 'pipe', 'inherit'],
    env: { ...process.env, COPYFILE_DISABLE: '1' },
  });
  const done = new Promise<void>((resolve, reject) => {
    tar.on('error', reject);
    tar.on('close', (exitCode) =>
      exitCode === 0 ? resolve() : reject(new CommandError(command, exitCode, '')),
    );
  });
  return { stream: tar.stdout, done };
}

/**
 * Replaces the app staged in `instance` with `bundle` and `app`'s config, laid
 * out as a TV package's `res` directory with the ICU data, to run on the
 * runtime build `runtime` names. `log` streams the app's logs to this Mac's
 * log server.
 */
export async function stageApp(
  instance: SimulatorInstance,
  app: AppProject,
  bundle: SimulatorBundle,
  log: LogEndpoint | null,
  runtime: SimulatorRuntime,
): Promise<void> {
  const dir = await mkdtemp(join(tmpdir(), 'rn-tizen-simulator-'));
  try {
    await writeResources(dir, app, bundle, log, runtime.icuData);
    const tar = archive(dir);
    await Promise.all([
      guestSpawn(['bash', guestScript('stage')], {
        stdin: tar.stream,
        env: { ...instanceEnv(instance), RN_TIZEN_SIMULATOR_RUNTIME: runtime.key },
      }),
      tar.done,
    ]);
  } finally {
    await rm(dir, { recursive: true, force: true });
  }
}
