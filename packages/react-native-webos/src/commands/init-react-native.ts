import { formatPath } from '../cli/format.ts';
import type { Reporter } from '../cli/reporter.ts';
import type { OverrideSetup } from '../dependencies/override-setup.ts';
import {
  duplicateReactNativeError,
  reactNativeCopies,
} from '../dependencies/react-native-copies.ts';
import { setUpReactNative, type ReactNativeSetup } from '../dependencies/set-up-react-native.ts';

function reportOverride(reporter: Reporter, override: OverrideSetup, cwd: string): void {
  const file = formatPath(override.file, cwd);
  if (override.kind === 'manual') {
    reporter.field('Override', `${file} cannot be changed safely: add these lines`);
    for (const line of override.lines) reporter.line(`  ${line}`);
    return;
  }
  const done =
    override.kind === 'set' ? 'overrides react-native' : 'overrides react-native already';
  reporter.field('Override', `${file} ${done}`);
}

function where(root: string, cwd: string): string {
  const path = formatPath(root, cwd);
  return path === '' ? '' : ` in ${path}`;
}

function report(reporter: Reporter, setup: ReactNativeSetup, cwd: string): void {
  const file = formatPath(setup.alias.file, cwd);
  const done = setup.alias.kind === 'set' ? 'now names' : 'names';
  reporter.field('React Native', `${file} ${done} ${setup.alias.spec}`);
  reportOverride(reporter, setup.override, cwd);
}

/**
 * Sets the app's react-native-tvos alias and its override, then runs the
 * install when anything changed or the app loads more than one react-native,
 * and fails unless one copy is left.
 */
export async function installReactNative(
  reporter: Reporter,
  root: string,
  cwd: string,
  skipInstall: boolean,
): Promise<void> {
  const setup = setUpReactNative(root);
  report(reporter, setup, cwd);
  const { manager } = setup;
  const changed = setup.alias.kind === 'set' || setup.override.kind === 'set' || setup.unlocked;
  if (!changed && reactNativeCopies(root).length < 2) return;
  const install = [manager.name, 'install'];
  if (skipInstall) {
    reporter.note(`Run ${install.join(' ')}${where(manager.root, cwd)} to apply it.`);
    return;
  }
  await reporter.step(
    'Install',
    (task) => task.run(install, { cwd: manager.root }),
    () => install.join(' '),
  );
  const copies = reactNativeCopies(root);
  if (copies.length > 1) {
    throw duplicateReactNativeError(
      root,
      copies,
      `Delete node_modules${where(manager.root, cwd)}, then run ${install.join(' ')} again.`,
    );
  }
}
