import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { ensureWebOSSdk } from '../sysroot/webos-sdk.ts';

/** `rn-webos setup`: fetches the SDK a TV build compiles against. */
export const setupCommand: Command = {
  name: 'setup',
  summary: 'Download the SDK TV builds compile against (package does this on its own)',
  options: COMMON_OPTIONS,
  examples: ['rn-webos setup'],
  async run(args) {
    const values = parseCommandArgs('setup', args, COMMON_OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const sdk = await reporter.step(
      'webOS SDK',
      (task) => ensureWebOSSdk(task),
      (found) =>
        found.reused ? `${found.version}, already set up` : `${found.version}, extracted`,
    );
    reporter.field('SDK', sdk.path);
  },
};
