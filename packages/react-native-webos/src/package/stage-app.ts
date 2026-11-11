import { chmod, copyFile, cp, mkdir, rm, writeFile } from 'node:fs/promises';
import { basename, join } from 'node:path';
import { findPackage } from '../app/app-packages.ts';
import type { AppProject } from '../app/app-project.ts';
import { claimWebOSDir } from '../app/webos-dir.ts';
import { BYTECODE_FILE, type AppBundle } from '../bundle/bundle-app.ts';
import { NOTICES_FILE } from '../package-paths.ts';
import { RUNTIME_EXECUTABLE } from '../runtime/build-runtime.ts';
import { writeAppConfig } from './app-config-file.ts';
import { writeAppIcons, writeSplash } from './app-icons.ts';
import { appInfo, APPINFO_FILE } from './appinfo.ts';
import { RUNTIME_CONFIG_FILE, runtimeConfig, type LogEndpoint } from './runtime-config.ts';
import { stripExecutable } from './strip-executable.ts';

/** The directory, in the app's `webos/`, that a package installs as `usr/palm/applications/<id>`. */
export const APP_DIR = 'app';

/** The directory the host reads the bundle, its config and its data files from. */
export const RESOURCE_DIR = 'res';

/** The CA bundle the runtime's TLS trusts, in the resource directory. */
export const CA_BUNDLE_FILE = 'cacert.pem';

// An app does not own its install directory on a TV, so LG's Flutter tool ships this
// empty world-writable one for its data (docs/reference/webos-tv-platform.md, section 3.1).
const APP_DATA_DIR = 'appdata';

const BUILD_DIR = 'build';

const VIDEO_PACKAGE = 'expo-video';

interface AppDirInputs {
  readonly app: AppProject;
  /** The `react-native-webos` executable built for the TV. */
  readonly runtime: string;
  readonly bundle: AppBundle;
  readonly log: LogEndpoint | null;
  /** The ICU data the runtime reads, `icudt<major>l.dat`. */
  readonly icuData: string;
  /** Mozilla's root certificates, as one PEM file. */
  readonly caBundle: string;
}

/**
 * Where a build of the app keeps the executable it packaged as it was linked,
 * with the symbols the packaged copy lost, for naming the frames of a crash.
 */
export function stagedExecutable(webosDir: string): string {
  return join(webosDir, BUILD_DIR, RUNTIME_EXECUTABLE);
}

async function writeResources(inputs: AppDirInputs, res: string): Promise<void> {
  await mkdir(res, { recursive: true });
  await copyFile(inputs.bundle.bytecode, join(res, BYTECODE_FILE));
  await copyFile(inputs.icuData, join(res, basename(inputs.icuData)));
  await copyFile(inputs.caBundle, join(res, CA_BUNDLE_FILE));
  await copyFile(NOTICES_FILE, join(res, basename(NOTICES_FILE)));
  await cp(inputs.bundle.assetsDir, join(res, 'assets'), { recursive: true });
  const config = runtimeConfig(
    BYTECODE_FILE,
    inputs.log,
    inputs.app.config,
    inputs.bundle.bundleUrl,
  );
  await writeFile(join(res, RUNTIME_CONFIG_FILE), `${JSON.stringify(config, null, 2)}\n`);
  await writeAppConfig(inputs.app.root, res);
}

/**
 * Writes the app directory of a package into the app's `webos/app`, replacing
 * the last one: `appinfo.json`, the executable without its symbols, the icons, the splash image
 * when the app has one, the resource directory and an empty `appdata`. The
 * `webos/` directory ignores itself, so it never reaches git.
 */
export async function stageAppDir(inputs: AppDirInputs): Promise<string> {
  const { app } = inputs;
  claimWebOSDir(app.webosDir);
  const dir = join(app.webosDir, APP_DIR);
  await rm(dir, { recursive: true, force: true });
  await mkdir(join(dir, APP_DATA_DIR), { recursive: true });
  await writeAppIcons(app, dir);
  const hasSplash = await writeSplash(app, dir);
  const executable = join(dir, RUNTIME_EXECUTABLE);
  await mkdir(join(app.webosDir, BUILD_DIR), { recursive: true });
  await copyFile(inputs.runtime, stagedExecutable(app.webosDir));
  await copyFile(inputs.runtime, executable);
  stripExecutable(executable);
  await chmod(executable, 0o755);
  await writeResources(inputs, join(dir, RESOURCE_DIR));
  const info = appInfo(app.config, {
    hasSplash,
    showsVideo: findPackage(app.root, VIDEO_PACKAGE) !== null,
  });
  await writeFile(join(dir, APPINFO_FILE), `${JSON.stringify(info, null, 2)}\n`);
  return dir;
}
