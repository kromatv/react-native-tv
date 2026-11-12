import { statSync } from 'node:fs';
import { join, relative } from 'node:path';
import type { AppProject } from '../app/app-project.ts';
import { claimWebOSDir } from '../app/webos-dir.ts';
import { bundleApp, type AppBundle } from '../bundle/bundle-app.ts';
import { formatSize } from '../cli/format.ts';
import type { Reporter } from '../cli/reporter.ts';
import { singleReactNativeIssue } from '../dependencies/react-native-copies.ts';
import { webosMetroIssue } from '../metro/metro-platforms.ts';
import { devBuildHost, devServerBundleStep } from '../package/dev-server-bundle.ts';
import { writeIpk } from '../package/ipk.ts';
import type { LogEndpoint } from '../package/runtime-config.ts';
import { stageAppDir } from '../package/stage-app.ts';
import { ensureCaBundle } from '../runtime/ca-bundle.ts';
import { ensureIcuData } from '../runtime/icu-data.ts';
import { runtimeSteps } from './runtime-steps.ts';

interface PackageRequest {
  readonly app: AppProject;
  /** Where a development build streams its logs; null makes a release build. */
  readonly log: LogEndpoint | null;
  /** The `expo start` origin a development build loads its bundle from. */
  readonly devServer?: string;
}

/** Where the bundle of an app is written before it is packaged. */
export function bundleDir(app: AppProject): string {
  return join(app.webosDir, 'build', 'bundle');
}

/** Bundles the app into Hermes bytecode, as one step, into `outDir` or else the app's `webos/`. */
export function bundleStep(
  reporter: Reporter,
  app: AppProject,
  outDir?: string,
): Promise<AppBundle> {
  return reporter.step(
    'JavaScript bundle',
    (task) => {
      if (outDir !== undefined) return bundleApp(task, app, outDir);
      claimWebOSDir(app.webosDir);
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

/** A built package and the app it installs. */
export interface BuiltPackage {
  readonly ipk: string;
  readonly appId: string;
}

/**
 * Fails, a step each, unless the app loads one react-native and its Metro
 * config bundles for webOS.
 */
export async function appSetupSteps(reporter: Reporter, app: AppProject): Promise<void> {
  await reporter.step(
    'React Native',
    () => {
      const issue = singleReactNativeIssue(app.root);
      return issue === null ? Promise.resolve() : Promise.reject(issue);
    },
    () => `${app.packages.reactNative.name} ${app.packages.reactNative.version}`,
  );
  await reporter.step(
    'Metro config',
    () => {
      const issue = webosMetroIssue(app.root);
      return issue === null ? Promise.resolve() : Promise.reject(issue);
    },
    () => 'knows webos',
  );
}

/**
 * Builds an `.ipk` from scratch or incrementally: the native runtime, the
 * bytecode bundle, the ICU data and the CA bundle, the app directory in the
 * app's `webos/`, then the package beside it. Fails fast, before any
 * compiling, when the app loads more than one react-native or Metro cannot
 * bundle for webOS.
 */
export async function packageSteps(
  reporter: Reporter,
  request: PackageRequest,
): Promise<BuiltPackage> {
  const { app } = request;
  claimWebOSDir(app.webosDir);
  await appSetupSteps(reporter, app);
  const flavour = request.log === null ? 'release' : 'development';
  const runtime = await runtimeSteps(reporter, app, flavour);
  const bundle =
    request.devServer === undefined
      ? await bundleStep(reporter, app)
      : await devServerBundleStep(reporter, app, bundleDir(app), request.devServer, () =>
          devBuildHost(request.log),
        );
  const icuData = await icuDataStep(reporter, app);
  const caBundle = await reporter.step(
    'CA certificates',
    (task) => ensureCaBundle(task),
    (file) => formatSize(statSync(file).size),
  );
  const appDir = await reporter.step(
    'App directory',
    () => stageAppDir({ app, runtime, bundle, log: request.log, icuData, caBundle }),
    (dir) => relative(process.cwd(), dir) || '.',
  );
  const written = await reporter.step(
    'Package',
    () => writeIpk({ appDir, outDir: app.webosDir }),
    (ipk) => `${flavour} runtime, installs ${formatSize(ipk.installedSize)}`,
  );
  return { ipk: written.file, appId: written.appId };
}
