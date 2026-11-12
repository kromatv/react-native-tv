import { runnablePlatforms } from '../expo-cli/extend-expo-cli.ts';
import type { PackageManager } from '../expo-patch/package-manager.ts';
import {
  patchExpoCli,
  type ExpoPatchApplied,
  type ExpoPatchOutcome,
} from '../expo-patch/patch-expo-cli.ts';
import type { CliIo } from './cli-io.ts';

const WITHOUT_PATCH = 'rn-tv run:<platform> works without the patch.';
const AFTER_UPGRADE = 'Run rn-tv expo-patch again after Expo CLI changes version.';

const POSTINSTALL: Readonly<Partial<Record<PackageManager, string>>> = {
  npm: 'npm has no patch feature, so the postinstall script applies the patch after an install. npm install <package> runs no postinstall: run rn-tv expo-patch again after Expo CLI changes version.',
  'yarn-classic':
    'Yarn 1 has no patch feature, so the postinstall script applies the patch after every install.',
};

function commands(projectRoot: string): string {
  const names = [...runnablePlatforms(projectRoot).keys()].map((name) => `npx expo run:${name}`);
  return names.length === 0
    ? 'the run command of each TV platform package this app adds'
    : names.join(', ');
}

function failure(outcome: Exclude<ExpoPatchOutcome, ExpoPatchApplied>): string {
  switch (outcome.state) {
    case 'no-expo':
      return 'This app has no Expo CLI: install expo first.';
    case 'unknown-layout':
      return `Expo CLI ${outcome.version} is laid out in a way this version of @kromatv/react-native-tv does not know. ${WITHOUT_PATCH}`;
    case 'no-lockfile':
      return `This app has no lockfile, so no package manager to patch Expo CLI with. ${WITHOUT_PATCH}`;
  }
}

function afterUpgrade({ manager, version }: ExpoPatchApplied): string {
  return manager === 'pnpm'
    ? `pnpm refuses to install while a patch is for a version that is gone, unless allowUnusedPatches is set: before an Expo upgrade run pnpm patch-remove @expo/cli@${version}, and rn-tv expo-patch after it.`
    : AFTER_UPGRADE;
}

function toCommit(applied: ExpoPatchApplied, projectRoot: string): string {
  const { manager, changed, installDir } = applied;
  if (changed.length === 0) return '';
  const files = `Commit ${changed.join(', ')} in ${installDir}`;
  const postinstall = POSTINSTALL[manager];
  if (postinstall === undefined) return `${files}. ${afterUpgrade(applied)}\n`;
  const inWorkspace =
    installDir === projectRoot
      ? ''
      : ` An install from ${installDir} runs the postinstall of that directory only: run rn-tv expo-patch here after it.`;
  return `${files}: ${postinstall}${inWorkspace}\n`;
}

function report(applied: ExpoPatchApplied, projectRoot: string): string {
  const { state, version, manager, removed } = applied;
  const headline =
    state === 'patched'
      ? `Expo CLI ${version} now runs: ${commands(projectRoot)}\n`
      : `Expo CLI ${version} has the patch already: ${commands(projectRoot)}\n`;
  const old =
    removed.length === 0
      ? ''
      : `Removed the patch of Expo CLI ${removed.join(', ')}, which is no longer installed.\n`;
  const commit = toCommit(applied, projectRoot);
  const blocked =
    manager === 'pnpm' && commit === ''
      ? `If pnpm refuses to install because this patch is unused, an Expo upgrade is waiting: run pnpm patch-remove @expo/cli@${version}, install, then rn-tv expo-patch.\n`
      : '';
  return headline + old + commit + blocked;
}

function failed(io: CliIo, reason: string): number {
  io.err(`${reason}\n`);
  return io.env?.npm_lifecycle_event === 'postinstall' ? 0 : 1;
}

/**
 * `rn-tv expo-patch`: adds the TV commands to the app's Expo CLI, and says
 * what to commit. Run as an app's `postinstall` it says why it could not and
 * succeeds all the same: a patch it cannot apply must not fail an install.
 */
export function expoPatchCommand(io: CliIo): number {
  let outcome: ExpoPatchOutcome;
  try {
    outcome = patchExpoCli(io.cwd, io.runManager);
  } catch (error) {
    return failed(io, error instanceof Error ? error.message : String(error));
  }
  if (outcome.state !== 'patched' && outcome.state !== 'present')
    return failed(io, failure(outcome));
  io.out(report(outcome, io.cwd));
  return 0;
}
