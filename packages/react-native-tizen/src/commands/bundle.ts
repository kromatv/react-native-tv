import { resolve } from 'node:path';
import { loadAppProject } from '../app/app-project.ts';
import {
  COMMON_OPTIONS,
  ENTRY_OPTIONS,
  entryPath,
  parseCommandArgs,
  PROJECT_OPTIONS,
  type Command,
} from '../cli/command.ts';
import { formatPath } from '../cli/format.ts';
import { Reporter } from '../cli/reporter.ts';
import { bundleStep, appSetupSteps } from '../pipeline/package-steps.ts';

const OPTIONS = {
  out: {
    type: 'string',
    placeholder: '<dir>',
    description: 'Where to write the bundle (default: tizen/build/bundle in the app)',
  },
  ...PROJECT_OPTIONS,
  ...ENTRY_OPTIONS,
  ...COMMON_OPTIONS,
} as const;

/** `rn-tizen bundle`: bundles the app's JavaScript and compiles it to Hermes bytecode. */
export const bundleCommand: Command = {
  name: 'bundle',
  summary: "Bundle the app's JavaScript with Metro and compile it to Hermes bytecode",
  options: OPTIONS,
  examples: [
    'rn-tizen bundle',
    'rn-tizen bundle --out dist/tizen',
    'rn-tizen bundle --entry src/screens/svg/main.ts',
  ],
  async run(args, context) {
    const values = parseCommandArgs('bundle', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const app = loadAppProject(
      resolve(context.cwd, values.project ?? '.'),
      entryPath(values, context),
    );
    const outDir = values.out === undefined ? undefined : resolve(context.cwd, values.out);
    await appSetupSteps(reporter, app);
    const bundle = await bundleStep(reporter, app, outDir);
    reporter.field('Bytecode', formatPath(bundle.bytecode, context.cwd));
    reporter.field('Assets', formatPath(bundle.assetsDir, context.cwd));
  },
};
