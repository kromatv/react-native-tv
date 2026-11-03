import { existsSync } from 'node:fs';
import { copyFile, cp, mkdir, readFile, readdir, rm, writeFile } from 'node:fs/promises';
import { basename, extname, join, resolve } from 'node:path';
import type { AppProject } from '../app/app-project.ts';
import type { AppBundle } from '../bundle/bundle-app.ts';
import { BYTECODE_FILE } from '../bundle/bundle-app.ts';
import { FixableError } from '../errors.ts';
import { NOTICES_FILE, SHELL_DIR } from '../package-paths.ts';
import { RUNTIME_LIBRARY } from '../runtime/build-runtime.ts';
import type { PlatformProfile } from '../tizen/platform-profiles.ts';
import { writeAppConfig } from './app-config-file.ts';
import { RUNTIME_CONFIG_FILE, runtimeConfig, type LogEndpoint } from './runtime-config.ts';
import { hasPreviewService, PREVIEW_SERVICE_DIR } from './feature-manifest.ts';
import {
  manifestValues,
  projectValues,
  renderTemplate,
  serviceManifestValues,
} from './templates.ts';

interface TizenProjectInputs {
  readonly app: AppProject;
  readonly profile: PlatformProfile;
  readonly runtimeLibrary: string;
  readonly bundle: AppBundle;
  readonly log: LogEndpoint | null;
  /** The ICU data the runtime reads, `icudt<major>l.dat`. */
  readonly icuData: string;
}

const PROJECT_FILE = 'ReactNativeTizen.csproj';
const MANIFEST_FILE = 'tizen-manifest.xml';
const DEFAULT_ICON = join(SHELL_DIR, 'icon.png');

function iconPath(app: AppProject): string {
  if (app.config.icon === undefined) return DEFAULT_ICON;
  const icon = resolve(app.root, app.config.icon);
  if (!existsSync(icon)) {
    throw new FixableError(
      `The app icon ${app.config.icon} does not exist`,
      'Fix expo.icon or expo.tizen.icon in app.json.',
    );
  }
  if (extname(icon).toLowerCase() !== '.png') {
    throw new FixableError(
      `The app icon ${app.config.icon} is not a PNG`,
      'Tizen only takes a PNG icon: point expo.tizen.icon at one.',
    );
  }
  return icon;
}

async function renderShellFile(
  name: string,
  values: Readonly<Record<string, string>>,
  dir: string,
  templateDir = SHELL_DIR,
) {
  const template = await readFile(join(templateDir, name), 'utf8');
  await writeFile(join(dir, name), renderTemplate(template, values));
}

async function copyShellSources(from: string, dir: string): Promise<void> {
  for (const name of await readdir(from)) {
    if (name.endsWith('.cs')) await copyFile(join(from, name), join(dir, name));
  }
}

async function writePreviewService(inputs: TizenProjectInputs, dir: string): Promise<void> {
  const serviceDir = join(dir, PREVIEW_SERVICE_DIR);
  await rm(serviceDir, { recursive: true, force: true });
  if (!hasPreviewService(inputs.app.config.features)) return;
  const templateDir = join(SHELL_DIR, 'preview-service');
  await mkdir(serviceDir, { recursive: true });
  const project = projectValues(inputs.profile, inputs.app.config);
  await renderShellFile('PreviewService.csproj', project, serviceDir, templateDir);
  const manifest = serviceManifestValues(inputs.app.config, inputs.profile);
  await renderShellFile(MANIFEST_FILE, manifest, serviceDir, templateDir);
  await copyShellSources(templateDir, serviceDir);
}

async function writeResources(inputs: TizenProjectInputs, dir: string): Promise<void> {
  const res = join(dir, 'res');
  await rm(res, { recursive: true, force: true });
  await mkdir(res, { recursive: true });
  await copyFile(inputs.bundle.bytecode, join(res, BYTECODE_FILE));
  await copyFile(inputs.icuData, join(res, basename(inputs.icuData)));
  await copyFile(NOTICES_FILE, join(res, basename(NOTICES_FILE)));
  await cp(inputs.bundle.assetsDir, join(res, 'assets'), { recursive: true });
  const config = runtimeConfig(BYTECODE_FILE, inputs.log, inputs.app.config);
  await writeFile(join(res, RUNTIME_CONFIG_FILE), `${JSON.stringify(config, null, 2)}\n`);
  await writeAppConfig(inputs.app.root, res);
}

/**
 * Generates the app's `tizen/` directory, Expo prebuild style: the .NET shell
 * from this package's templates, the native runtime, the bytecode bundle and
 * the icon. The directory ignores itself, so it never reaches git.
 */
export async function writeTizenProject(inputs: TizenProjectInputs): Promise<string> {
  const dir = inputs.app.tizenDir;
  await mkdir(join(dir, 'lib'), { recursive: true });
  await mkdir(join(dir, 'shared', 'res'), { recursive: true });
  await writeFile(join(dir, '.gitignore'), '*\n');
  await renderShellFile(PROJECT_FILE, projectValues(inputs.profile, inputs.app.config), dir);
  await renderShellFile(MANIFEST_FILE, manifestValues(inputs.app.config, inputs.profile), dir);
  await copyShellSources(SHELL_DIR, dir);
  await writePreviewService(inputs, dir);
  await copyFile(inputs.runtimeLibrary, join(dir, 'lib', RUNTIME_LIBRARY));
  await copyFile(iconPath(inputs.app), join(dir, 'shared', 'res', 'icon.png'));
  await writeResources(inputs, dir);
  return dir;
}
