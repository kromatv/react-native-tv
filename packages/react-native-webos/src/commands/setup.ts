import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { chooseTarget, TARGET_OPTIONS } from '../cli/target-options.ts';
import { setupTarget } from '../sysroot/setup-target.ts';
import { locateWebOSSdk } from '../webos/webos-sdk.ts';

const OPTIONS = { ...TARGET_OPTIONS, ...COMMON_OPTIONS };

/** `rn-webos setup`: makes a webOS target buildable. */
export const setupCommand: Command = {
  name: 'setup',
  summary: 'Prepare a webOS target for native builds (package does this on its own)',
  options: OPTIONS,
  examples: ['rn-webos setup', 'rn-webos setup --arch aarch64 --platform 9.0'],
  async run(args) {
    const values = parseCommandArgs('setup', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const target = chooseTarget({ command: 'setup', arch: values.arch, platform: values.platform });
    const sdk = locateWebOSSdk();
    const sysroot = await reporter.step(
      `webOS ${target.platformVersion} for ${target.arch}`,
      (task) => setupTarget(task, sdk, target),
      (mirrored) =>
        mirrored.reused ? 'already set up' : `${mirrored.patchedLibraries} libraries patched`,
    );
    reporter.field('Sysroot', sysroot.path);
  },
};
