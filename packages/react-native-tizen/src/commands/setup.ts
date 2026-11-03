import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { chooseTarget, TARGET_OPTIONS } from '../cli/target-options.ts';
import { setupTarget } from '../sysroot/setup-target.ts';
import { locateTizenSdk } from '../tizen/tizen-sdk.ts';

const OPTIONS = { ...TARGET_OPTIONS, ...COMMON_OPTIONS };

/** `rn-tizen setup`: makes a Tizen target buildable. */
export const setupCommand: Command = {
  name: 'setup',
  summary: 'Prepare a Tizen target for native builds (package does this on its own)',
  options: OPTIONS,
  examples: ['rn-tizen setup', 'rn-tizen setup --arch aarch64 --platform 9.0'],
  async run(args) {
    const values = parseCommandArgs('setup', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const target = chooseTarget({ command: 'setup', arch: values.arch, platform: values.platform });
    const sdk = locateTizenSdk();
    const sysroot = await reporter.step(
      `Tizen ${target.platformVersion} for ${target.arch}`,
      (task) => setupTarget(task, sdk, target),
      (mirrored) =>
        mirrored.reused ? 'already set up' : `${mirrored.patchedLibraries} libraries patched`,
    );
    reporter.field('Sysroot', sysroot.path);
  },
};
