import { resolve } from 'node:path';
import { loadAppProject } from '../app/app-project.ts';
import {
  COMMON_OPTIONS,
  parseCommandArgs,
  PROJECT_OPTIONS,
  type Command,
  type OptionSpecs,
} from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { runtimeSteps } from '../pipeline/runtime-steps.ts';

const OPTIONS = {
  release: {
    type: 'boolean',
    description: 'Compile the runtime release builds get, with no dev server tooling',
  },
  ...PROJECT_OPTIONS,
  ...COMMON_OPTIONS,
} as const satisfies OptionSpecs;

/** `rn-webos build-runtime`: compiles the `react-native-webos` executable for an app. */
export const buildRuntimeCommand: Command = {
  name: 'build-runtime',
  summary: "Compile the TV runtime against the app's react-native and Expo",
  options: OPTIONS,
  examples: [
    'rn-webos build-runtime',
    'rn-webos build-runtime --release',
    'rn-webos build-runtime --verbose',
  ],
  async run(args, context) {
    const values = parseCommandArgs('build-runtime', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const app = loadAppProject(resolve(context.cwd, values.project ?? '.'));
    const flavour = values.release === true ? 'release' : 'development';
    const executable = await runtimeSteps(reporter, app, flavour);
    reporter.field('Runtime', executable);
  },
};
