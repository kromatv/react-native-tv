import { createHash } from 'node:crypto';
import { existsSync, realpathSync } from 'node:fs';
import { mkdir } from 'node:fs/promises';
import { join } from 'node:path';
import { TOOLCHAIN_FILE } from '../package-paths.ts';
import { stampAt } from '../stamp.ts';
import type { Task } from '../task.ts';
import type { TizenSdk } from '../tizen/tizen-sdk.ts';
import { rootstrapFor, type TizenTarget } from '../tizen/target.ts';
import { ninjaProgress } from './ninja-progress.ts';

interface CMakeProject {
  readonly source: string;
  readonly build: string;
  readonly definitions: Readonly<Record<string, string>>;
  readonly targets?: readonly string[];
}

/**
 * A short key for where `paths` are on disk. A CMake build directory belongs to
 * one source tree, so a build of sources two checkouts both hold is named with it.
 */
export function sourcesKey(...paths: readonly string[]): string {
  return createHash('sha256')
    .update(paths.map((path) => realpathSync(path)).join('\0'))
    .digest('hex')
    .slice(0, 12);
}

/** The definitions that cross-compile a project for one Tizen target. */
export function tizenDefinitions(sdk: TizenSdk, target: TizenTarget): Record<string, string> {
  const rootstrap = rootstrapFor(target);
  return {
    CMAKE_TOOLCHAIN_FILE: TOOLCHAIN_FILE,
    CMAKE_BUILD_TYPE: 'Release',
    TIZEN_ARCH: target.arch,
    TIZEN_PLATFORM_VERSION: target.platformVersion,
    TIZEN_ROOTSTRAP_VERSION: rootstrap.platformVersion,
    TIZEN_ROOTSTRAP_PROFILE: rootstrap.profile,
    TIZEN_SDK_DIR: sdk.root,
  };
}

async function configure(task: Task, project: CMakeProject, flags: readonly string[]) {
  const stamp = stampAt(join(project.build, '.rn-tizen-configure.json'));
  const fingerprint = JSON.stringify({ source: project.source, flags });
  const generated = existsSync(join(project.build, 'build.ninja'));
  if (generated && (await stamp.matches(fingerprint))) return;
  await mkdir(project.build, { recursive: true });
  await stamp.clear();
  task.progress('configuring');
  await task.run(['cmake', '-S', project.source, '-B', project.build, '-G', 'Ninja', ...flags]);
  await stamp.record(fingerprint);
}

/**
 * Configures a project when its definitions changed since the last configure,
 * then builds it, reporting Ninja's progress to the task.
 */
export async function buildCMakeProject(task: Task, project: CMakeProject): Promise<void> {
  const flags = Object.entries(project.definitions).map(([name, value]) => `-D${name}=${value}`);
  await configure(task, project, flags);
  const targetFlags = project.targets?.length ? ['--target', ...project.targets] : [];
  await task.run(['cmake', '--build', project.build, ...targetFlags], {
    onLine: (line) => {
      const progress = ninjaProgress(line);
      if (progress !== null) task.progress(progress);
    },
  });
}
