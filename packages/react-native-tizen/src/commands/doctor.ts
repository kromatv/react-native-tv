import { resolve } from 'node:path';
import { COMMON_OPTIONS, parseCommandArgs, PROJECT_OPTIONS, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { chooseTarget, TARGET_OPTIONS } from '../cli/target-options.ts';
import { checkApp } from '../doctor/app-check.ts';
import { failed, passed, type CheckResult, type CheckStatus } from '../doctor/check-result.ts';
import {
  checkDevices,
  checkGcc,
  checkRootstrap,
  checkSecurityProfile,
} from '../doctor/tizen-checks.ts';
import {
  checkCcache,
  checkCMake,
  checkDotnet,
  checkLlvm,
  checkNinja,
  checkNode,
} from '../doctor/toolchain-checks.ts';
import { FixableError } from '../errors.ts';
import type { Task } from '../task.ts';
import { locateTizenSdk, type TizenSdk } from '../tizen/tizen-sdk.ts';
import type { TizenTarget } from '../tizen/target.ts';

const OPTIONS = { ...TARGET_OPTIONS, ...PROJECT_OPTIONS, ...COMMON_OPTIONS };
const TITLE_WIDTH = 18;

const MARKS: Record<CheckStatus, readonly [string, 'green' | 'yellow' | 'red' | 'gray']> = {
  ok: ['✓', 'green'],
  warn: ['!', 'yellow'],
  fail: ['✗', 'red'],
  skip: ['-', 'gray'],
};

function sdkChecks(task: Task, sdk: TizenSdk | null, target: TizenTarget): Promise<CheckResult>[] {
  const titles = ['GCC runtime', 'Rootstrap', 'Security profile'];
  if (sdk === null) {
    return titles.map((title) =>
      Promise.resolve({ title, status: 'skip', detail: 'needs Tizen Studio' } as const),
    );
  }
  return [checkGcc(sdk, target), checkRootstrap(sdk, target), checkSecurityProfile(task, sdk)];
}

function locate(): { sdk: TizenSdk | null; result: CheckResult } {
  try {
    const sdk = locateTizenSdk();
    return { sdk, result: passed('Tizen Studio', sdk.root) };
  } catch (error) {
    if (!(error instanceof FixableError)) throw error;
    return { sdk: null, result: failed('Tizen Studio', error.message, error.hint) };
  }
}

function print(reporter: Reporter, result: CheckResult): void {
  const [mark, color] = MARKS[result.status];
  const title = result.title.padEnd(TITLE_WIDTH);
  const detail = result.status === 'ok' ? reporter.paint('gray', result.detail) : result.detail;
  reporter.line(`${reporter.paint(color, mark)} ${title}${detail}`);
  if (result.fix !== undefined) reporter.line(`    ${reporter.paint('cyan', result.fix)}`);
}

/** `rn-tizen doctor`: checks this machine can build, sign and install a package. */
export const doctorCommand: Command = {
  name: 'doctor',
  summary: 'Check this machine can build, sign and install for Samsung TVs',
  options: OPTIONS,
  examples: ['rn-tizen doctor', 'rn-tizen doctor --arch aarch64'],
  async run(args, context) {
    const values = parseCommandArgs('doctor', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const target = chooseTarget({
      command: 'doctor',
      arch: values.arch,
      platform: values.platform,
    });
    const task = reporter.task();
    const studio = locate();
    const results = await Promise.all([
      Promise.resolve(studio.result),
      ...sdkChecks(task, studio.sdk, target),
      checkDevices(),
      checkLlvm(task),
      checkCMake(task),
      checkNinja(task),
      checkDotnet(task),
      checkNode(task),
      checkCcache(),
      checkApp(resolve(context.cwd, values.project ?? '.')),
    ]);
    for (const result of results) print(reporter, result);
    const failures = results.filter((result) => result.status === 'fail').length;
    reporter.line();
    if (failures === 0) {
      reporter.line(
        reporter.paint(
          'green',
          `Ready to build for Tizen ${target.platformVersion} on ${target.arch}.`,
        ),
      );
      return;
    }
    reporter.line(
      reporter.paint('red', `${failures} ${failures === 1 ? 'check' : 'checks'} failed.`),
    );
    process.exitCode = 1;
  },
};
