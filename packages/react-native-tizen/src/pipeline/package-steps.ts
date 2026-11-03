import { statSync } from 'node:fs';
import { join, relative } from 'node:path';
import type { AppProject } from '../app/app-project.ts';
import { claimTizenDir } from '../app/tizen-dir.ts';
import { bundleApp, type AppBundle } from '../bundle/bundle-app.ts';
import { formatSize } from '../cli/format.ts';
import { ensureIcuData } from '../runtime/icu-data.ts';
import type { Reporter } from '../cli/reporter.ts';
import { buildDotnetProject } from '../package/dotnet-build.ts';
import { featureNotes } from '../package/feature-notes.ts';
import type { LogEndpoint } from '../package/runtime-config.ts';
import { writeTizenProject } from '../package/tizen-project.ts';
import type { PlatformProfile } from '../tizen/platform-profiles.ts';
import { signingProfile, signPackage } from '../tizen/tizen-cli.ts';
import type { TizenSdk } from '../tizen/tizen-sdk.ts';
import type { TizenTarget } from '../tizen/target.ts';
import { runtimeSteps } from './runtime-steps.ts';

interface PackageRequest {
  readonly sdk: TizenSdk;
  readonly app: AppProject;
  readonly target: TizenTarget;
  readonly shell: PlatformProfile;
  readonly log: LogEndpoint | null;
  readonly profile: string | undefined;
}

/** Where the bundle of an app is written before it is packaged. */
export function bundleDir(app: AppProject): string {
  return join(app.tizenDir, 'build', 'bundle');
}

/** Bundles the app into Hermes bytecode, as one step, into `outDir` or else the app's `tizen/`. */
export function bundleStep(
  reporter: Reporter,
  app: AppProject,
  outDir?: string,
): Promise<AppBundle> {
  return reporter.step(
    'JavaScript bundle',
    (task) => {
      if (outDir !== undefined) return bundleApp(task, app, outDir);
      claimTizenDir(app.tizenDir);
      return bundleApp(task, app, bundleDir(app));
    },
    (bundle) => formatSize(statSync(bundle.bytecode).size),
  );
}

/** The ICU data trimmed to the app's locales, as one step. */
function icuDataStep(reporter: Reporter, app: AppProject): Promise<string> {
  return reporter.step(
    'ICU data',
    (task) => ensureIcuData(task, app.config.locales),
    (file) => `${app.config.locales.join(', ')}, ${formatSize(statSync(file).size)}`,
  );
}

/**
 * Builds a signed `.tpk` from scratch or incrementally: the native runtime, the
 * bytecode bundle, the generated `tizen/` project, then .NET and signing.
 * Fails fast, before any compiling, when there is nothing to sign with.
 */
export async function packageSteps(reporter: Reporter, request: PackageRequest): Promise<string> {
  const { sdk, app, target, shell } = request;
  claimTizenDir(app.tizenDir);
  const profile = await reporter.step(
    'Security profile',
    (task) => signingProfile(task, sdk, request.profile ?? app.config.profile),
    (name) => name,
  );
  for (const note of featureNotes(app.config.features, request.log === null)) reporter.note(note);
  const runtimeLibrary = await runtimeSteps(reporter, sdk, app, target);
  const bundle = await bundleStep(reporter, app);
  const icuData = await icuDataStep(reporter, app);
  const projectDir = await reporter.step(
    'Tizen project',
    () =>
      writeTizenProject({ app, profile: shell, runtimeLibrary, bundle, log: request.log, icuData }),
    (dir) => relative(process.cwd(), dir) || '.',
  );
  const tpk = await reporter.step(
    '.NET build',
    (task) => buildDotnetProject(task, projectDir, app.config, shell),
    () => shell.targetFramework,
  );
  await reporter.step(
    'Signing',
    (task) => signPackage(task, sdk, tpk, profile),
    () => profile,
  );
  return tpk;
}
