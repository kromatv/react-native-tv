import { existsSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { ask, canAsk } from '../cli/ask.ts';
import { COMMON_OPTIONS, parseCommandArgs, PROJECT_OPTIONS, type Command } from '../cli/command.ts';
import { formatPath } from '../cli/format.ts';
import { packageRunner } from '../dependencies/package-manager.ts';
import { Reporter } from '../cli/reporter.ts';
import { readExpoConfig } from '../app/expo-config.ts';
import { addTizenPlugin, type PluginSetup } from '../config/add-tizen-plugin.ts';
import { packageIdIssue, suggestedPackageId } from '../config/package-id.ts';
import { isTizenPluginEntry } from '../config/plugin-entry.ts';
import { resolveTizenConfig } from '../config/tizen-config.ts';
import { TIZEN_PLUGIN } from '../config/tizen-settings.ts';
import { FixableError } from '../errors.ts';
import { setUpMetroConfig, type MetroSetup } from '../metro/set-up-metro-config.ts';
import { DEFAULT_TARGET, rootstrapFor } from '../tizen/target.ts';
import { installReactNative } from './init-react-native.ts';

const OPTIONS = {
  package: {
    type: 'string',
    placeholder: '<id>',
    description: "The app's Tizen package id, such as com.example.myapp (asked when left out)",
  },
  'no-install': {
    type: 'boolean',
    description: "Change package.json without running the package manager's install",
  },
  ...PROJECT_OPTIONS,
  ...COMMON_OPTIONS,
} as const;

function checkedPackageId(id: string): string {
  const issue = packageIdIssue(id);
  if (issue === null) return id;
  throw new FixableError(
    `${id} is not a Tizen package id: ${issue}`,
    'Pass --package com.example.myapp.',
  );
}

async function askPackageId(reporter: Reporter, suggestion: string): Promise<string> {
  const answer = await ask('Tizen package id', suggestion);
  const issue = packageIdIssue(answer);
  if (issue === null) return answer;
  reporter.line(reporter.paint('yellow', `${answer}: ${issue}`));
  return askPackageId(reporter, suggestion);
}

function choosePackageId(reporter: Reporter, given: string | undefined) {
  return async (expo: Record<string, unknown>): Promise<string> => {
    if (given !== undefined) return checkedPackageId(given);
    const suggestion = suggestedPackageId(expo);
    if (canAsk()) return askPackageId(reporter, suggestion);
    reporter.note(`Package id ${suggestion}; pass --package to choose another.`);
    return suggestion;
  };
}

function reportPlugin(reporter: Reporter, setup: PluginSetup, cwd: string): void {
  const file = formatPath(setup.file, cwd);
  if (setup.kind === 'dynamic') {
    reporter.field('App config', `${file} is evaluated by Expo: add this entry to its plugins`);
    reporter.line(`  ${setup.entry}`);
    return;
  }
  const done = {
    added: `added the ${TIZEN_PLUGIN} config plugin`,
    moved: `moved expo.tizen into the props of the ${TIZEN_PLUGIN} config plugin`,
    present: `lists the ${TIZEN_PLUGIN} config plugin already`,
  }[setup.kind];
  reporter.field('App config', `${file} ${done}`);
}

function reportMetro(reporter: Reporter, setup: MetroSetup, cwd: string): void {
  const file = formatPath(setup.file, cwd);
  if (setup.kind === 'manual') {
    reporter.field('Metro config', `${file} cannot be changed safely: add these two lines`);
    for (const line of setup.lines) reporter.line(`  ${line}`);
    return;
  }
  const done = {
    written: 'written',
    wrapped: 'wrapped with withTizen',
    present: 'uses withTizen already',
  }[setup.kind];
  reporter.field('Metro config', `${file} ${done}`);
}

function appFloor(root: string): string {
  try {
    const config = resolveTizenConfig(readExpoConfig(root), (name) =>
      isTizenPluginEntry(root, name),
    );
    return config.platform ?? DEFAULT_TARGET.platformVersion;
  } catch {
    return DEFAULT_TARGET.platformVersion;
  }
}

function reportRootstrap(reporter: Reporter, root: string): void {
  const floor = appFloor(root);
  const { sdkPackage } = rootstrapFor({ ...DEFAULT_TARGET, platformVersion: floor });
  reporter.note(
    `Builds for Tizen ${floor}, the app's floor, need ${sdkPackage} from Tizen Studio's Package Manager: ${packageRunner(root)} rn-tizen doctor checks it.`,
  );
}

/**
 * `rn-tizen init`: lists the config plugin in the app's config, wraps its Metro
 * config with `withTizen`, and installs one react-native-tvos.
 */
export const initCommand: Command = {
  name: 'init',
  summary:
    'Set an Expo app up for Tizen: the config plugin, withTizen in the Metro config, one react-native-tvos',
  options: OPTIONS,
  examples: ['npx rn-tizen init', 'npx rn-tizen init --package com.example.myapp'],
  async run(args, context) {
    const values = parseCommandArgs('init', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const root = resolve(context.cwd, values.project ?? '.');
    if (!existsSync(join(root, 'package.json'))) {
      throw new FixableError(
        `${root} has no package.json`,
        `Run ${packageRunner(root)} rn-tizen init in the app, or pass --project <dir>.`,
      );
    }
    reportPlugin(
      reporter,
      await addTizenPlugin(root, choosePackageId(reporter, values.package)),
      context.cwd,
    );
    reportMetro(reporter, setUpMetroConfig(root), context.cwd);
    await installReactNative(reporter, root, context.cwd, values['no-install'] ?? false);
    reporter.line();
    reportRootstrap(reporter, root);
    reporter.note(
      `Next: ${packageRunner(root)} expo start, then press t to open the app on the TV used last, or shift+t to pick a TV or the simulator.`,
    );
  },
};
