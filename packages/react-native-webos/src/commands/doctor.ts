import { resolve } from 'node:path';
import { COMMON_OPTIONS, parseCommandArgs, PROJECT_OPTIONS, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { checkApp } from '../doctor/app-check.ts';
import type { CheckResult, CheckStatus } from '../doctor/check-result.ts';
import { checkDevices } from '../doctor/webos-checks.ts';
import {
  checkCcache,
  checkCMake,
  checkLlvm,
  checkNinja,
  checkNode,
  checkSdk,
} from '../doctor/toolchain-checks.ts';

const OPTIONS = { ...PROJECT_OPTIONS, ...COMMON_OPTIONS };
const TITLE_WIDTH = 18;

const MARKS: Record<CheckStatus, readonly [string, 'green' | 'yellow' | 'red' | 'gray']> = {
  ok: ['✓', 'green'],
  warn: ['!', 'yellow'],
  fail: ['✗', 'red'],
  skip: ['-', 'gray'],
};

function print(reporter: Reporter, result: CheckResult): void {
  const [mark, color] = MARKS[result.status];
  const title = result.title.padEnd(TITLE_WIDTH);
  const detail = result.status === 'ok' ? reporter.paint('gray', result.detail) : result.detail;
  reporter.line(`${reporter.paint(color, mark)} ${title}${detail}`);
  if (result.fix !== undefined) reporter.line(`    ${reporter.paint('cyan', result.fix)}`);
}

/** `rn-webos doctor`: checks this machine can build a package and reach a TV. */
export const doctorCommand: Command = {
  name: 'doctor',
  summary: 'Check this machine can build and install for LG webOS TVs',
  options: OPTIONS,
  examples: ['rn-webos doctor', 'rn-webos doctor --project example'],
  async run(args, context) {
    const values = parseCommandArgs('doctor', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const appDir = resolve(context.cwd, values.project ?? '.');
    const task = reporter.task();
    const results = await Promise.all([
      checkSdk(),
      checkLlvm(task),
      checkCMake(task),
      checkNinja(task),
      checkNode(task),
      checkCcache(),
      checkDevices(),
      checkApp(appDir),
    ]);
    for (const result of results) print(reporter, result);
    const failures = results.filter((result) => result.status === 'fail').length;
    reporter.line();
    if (failures === 0) {
      reporter.line(reporter.paint('green', 'Ready to build for webOS TVs.'));
      return;
    }
    reporter.line(
      reporter.paint('red', `${failures} ${failures === 1 ? 'check' : 'checks'} failed.`),
    );
    process.exitCode = 1;
  },
};
