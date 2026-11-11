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
import { chooseTarget, TARGET_OPTIONS } from '../cli/target-options.ts';
import { runtimeSteps } from '../pipeline/runtime-steps.ts';
import { locateTizenSdk } from '../tizen/tizen-sdk.ts';

const OPTIONS = {
  ...TARGET_OPTIONS,
  release: {
    type: 'boolean',
    description: 'Compile the runtime release builds get, with no dev server tooling',
  },
  ...PROJECT_OPTIONS,
  ...COMMON_OPTIONS,
} as const satisfies OptionSpecs;

/** `rn-tizen build-runtime`: compiles `libreact-native-tizen.so` for an app. */
export const buildRuntimeCommand: Command = {
  name: 'build-runtime',
  summary: "Compile the native runtime against the app's react-native and Expo",
  options: OPTIONS,
  examples: [
    'rn-tizen build-runtime',
    'rn-tizen build-runtime --release',
    'rn-tizen build-runtime --arch aarch64 --verbose',
  ],
  async run(args, context) {
    const values = parseCommandArgs('build-runtime', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const app = loadAppProject(resolve(context.cwd, values.project ?? '.'));
    const target = chooseTarget({
      command: 'build-runtime',
      arch: values.arch,
      platform: values.platform,
      configPlatform: app.config.platform,
    });
    reporter.field('Target', `${target.arch}, Tizen ${target.platformVersion}`);
    const flavour = values.release === true ? 'release' : 'development';
    const library = await runtimeSteps(reporter, locateTizenSdk(), app, target, flavour);
    reporter.field('Runtime', library);
  },
};
