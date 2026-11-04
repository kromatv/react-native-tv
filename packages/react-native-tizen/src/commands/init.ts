import { existsSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { ask, canAsk } from '../cli/ask.ts';
import { COMMON_OPTIONS, parseCommandArgs, PROJECT_OPTIONS, type Command } from '../cli/command.ts';
import { formatPath } from '../cli/format.ts';
import { Reporter } from '../cli/reporter.ts';
import { addTizenPlugin, type PluginSetup } from '../config/add-tizen-plugin.ts';
import { packageIdIssue, suggestedPackageId } from '../config/package-id.ts';
import { TIZEN_PLUGIN } from '../config/tizen-settings.ts';
import { FixableError } from '../errors.ts';
import { setUpMetroConfig, type MetroSetup } from '../metro/set-up-metro-config.ts';

const OPTIONS = {
  package: {
    type: 'string',
    placeholder: '<id>',
    description: "The app's Tizen package id, such as com.example.myapp (asked when left out)",
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

/** `rn-tizen init`: lists the config plugin in the app's config and wraps its Metro config with `withTizen`. */
export const initCommand: Command = {
  name: 'init',
  summary:
    'Set an Expo app up for Tizen: the config plugin in app.json, withTizen in the Metro config',
  options: OPTIONS,
  examples: ['npx rn-tizen init', 'npx rn-tizen init --package com.example.myapp'],
  async run(args, context) {
    const values = parseCommandArgs('init', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const root = resolve(context.cwd, values.project ?? '.');
    if (!existsSync(join(root, 'package.json'))) {
      throw new FixableError(
        `${root} has no package.json`,
        'Run npx rn-tizen init in the app, or pass --project <dir>.',
      );
    }
    reportPlugin(
      reporter,
      await addTizenPlugin(root, choosePackageId(reporter, values.package)),
      context.cwd,
    );
    reportMetro(reporter, setUpMetroConfig(root), context.cwd);
    reporter.line();
    reporter.note(
      'Next: npx expo start, then press shift+m and pick Run on TV or Run in simulator.',
    );
  },
};
