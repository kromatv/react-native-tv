import { basename } from 'node:path';
import {
  bannerModel,
  DEFAULT_TOOL_PATH,
  DUID_COMMAND,
  installCommand,
  installProgress,
  installSucceeded,
  launchCommand,
  launchSucceeded,
  remotePackagePath,
  uninstallCommand,
  uninstallSucceeded,
} from './app-commands.ts';
import { readCapabilities, type DeviceCapabilities } from './capabilities.ts';
import { SdbConnection, type SdbAddress } from './sdb-connection.ts';
import { runShell } from './shell-command.ts';
import { pushFile } from './sync-push.ts';

const INSTALL_TIMEOUT_MS = 300_000;
const LAUNCH_TIMEOUT_MS = 60_000;
const DUID = /^[A-Z0-9]{8,32}$/;

/** What an install or a launch printed, when it did not report success. */
export class SdbCommandError extends Error {
  readonly output: string;

  constructor(message: string, output: string) {
    super(message);
    this.name = 'SdbCommandError';
    this.output = output.trim();
  }
}

export interface InstallProgress {
  readonly phase: 'push' | 'install';
  readonly percent: number;
}

/** A TV's sdb daemon, connected, with what it said about itself. */
export interface SdbDevice {
  readonly address: SdbAddress;
  readonly model: string | null;
  readonly capabilities: DeviceCapabilities | null;
  install(
    tpk: string,
    packageId: string,
    onProgress?: (progress: InstallProgress) => void,
  ): Promise<void>;
  launch(appId: string): Promise<void>;
  uninstall(packageId: string): Promise<void>;
  duid(): Promise<string | null>;
  close(): void;
}

async function install(
  connection: SdbConnection,
  capabilities: DeviceCapabilities | null,
  tpk: string,
  packageId: string,
  onProgress: (progress: InstallProgress) => void,
): Promise<void> {
  const remote = remotePackagePath(capabilities?.toolPath ?? DEFAULT_TOOL_PATH, basename(tpk));
  await pushFile(connection, tpk, remote, (sent, total) => {
    onProgress({ phase: 'push', percent: total === 0 ? 100 : Math.floor((sent * 100) / total) });
  });
  const output = await runShell(connection, installCommand(packageId, remote), {
    timeoutMs: INSTALL_TIMEOUT_MS,
    onLine: (line) => {
      const percent = installProgress(line);
      if (percent !== null) onProgress({ phase: 'install', percent });
    },
  });
  if (!installSucceeded(output)) {
    throw new SdbCommandError(`The TV did not install ${packageId}`, output);
  }
}

/** Connects to a TV and reads its capabilities. */
export async function openSdbDevice(address: SdbAddress, timeoutMs?: number): Promise<SdbDevice> {
  const connection = await SdbConnection.open(address, timeoutMs);
  let capabilities: DeviceCapabilities | null;
  try {
    capabilities = await readCapabilities(connection);
  } catch (error) {
    connection.close();
    throw error;
  }
  return {
    address,
    model: bannerModel(connection.banner),
    capabilities,
    install: (tpk, packageId, onProgress = () => undefined) =>
      install(connection, capabilities, tpk, packageId, onProgress),
    async launch(appId) {
      const output = await runShell(connection, launchCommand(appId), {
        timeoutMs: LAUNCH_TIMEOUT_MS,
      });
      if (!launchSucceeded(output)) {
        throw new SdbCommandError(`The TV did not launch ${appId}`, output);
      }
    },
    async uninstall(packageId) {
      const output = await runShell(connection, uninstallCommand(packageId), {
        timeoutMs: INSTALL_TIMEOUT_MS,
      });
      if (!uninstallSucceeded(output)) {
        throw new SdbCommandError(`The TV did not uninstall ${packageId}`, output);
      }
    },
    async duid() {
      const output = (await runShell(connection, DUID_COMMAND, { limit: 256 })).trim();
      return DUID.test(output) ? output : null;
    },
    close: () => connection.close(),
  };
}

/** Runs `work` on a fresh connection to the TV and closes it afterwards. */
export async function withSdbDevice<T>(
  address: SdbAddress,
  work: (device: SdbDevice) => Promise<T>,
  timeoutMs?: number,
): Promise<T> {
  const device = await openSdbDevice(address, timeoutMs);
  try {
    return await work(device);
  } finally {
    device.close();
  }
}
