import { existsSync } from 'node:fs';
import { mkdir, rename, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import type { Task } from '../task.ts';
import { buildCMakeProject } from './cmake.ts';
import { archiveFileName, pinnedDependency } from './dependencies.ts';
import { downloadVerified } from './download.ts';

/** Hermes compilers built for this machine, which the cross build imports. */
export interface HermesHostCompilers {
  readonly version: string;
  readonly importFile: string;
  readonly hermesc: string;
}

const HOST_DEFINITIONS = {
  CMAKE_BUILD_TYPE: 'Release',
  HERMES_ENABLE_TEST_SUITE: 'OFF',
  HERMES_BUILD_APPLE_FRAMEWORK: 'OFF',
  HERMES_ENABLE_DEBUGGER: 'OFF',
};

function hostPaths(version: string) {
  const root = join(cacheDir(), 'hermes', version);
  const build = join(root, 'host');
  return {
    source: join(root, 'source'),
    build,
    importFile: join(build, 'ImportHostCompilers.cmake'),
    hermesc: join(build, 'bin', 'hermesc'),
  };
}

/** Where the host compilers for the pinned Hermes live, built or not. */
export function hermesHostCompilers(): HermesHostCompilers & { readonly built: boolean } {
  const { version } = pinnedDependency('hermes');
  const paths = hostPaths(version);
  const built = existsSync(paths.importFile) && existsSync(paths.hermesc);
  return { version, importFile: paths.importFile, hermesc: paths.hermesc, built };
}

async function extractSource(task: Task, archive: string, source: string): Promise<void> {
  const staging = `${source}.staging-${process.pid}`;
  await rm(staging, { recursive: true, force: true });
  await mkdir(staging, { recursive: true });
  await task.run(['tar', '-xzf', archive, '-C', staging, '--strip-components=1']);
  await rm(source, { recursive: true, force: true });
  await rename(staging, source);
}

/**
 * Builds `hermesc` and `shermes` for this machine from the pinned Hermes
 * source, once per Hermes version. The archive is the one CMake downloads for
 * the cross build, so it is fetched once.
 */
export async function ensureHermesHostCompilers(task: Task): Promise<HermesHostCompilers> {
  const existing = hermesHostCompilers();
  if (existing.built) return existing;
  const pin = pinnedDependency('hermes');
  const paths = hostPaths(pin.version);
  if (!existsSync(join(paths.source, 'CMakeLists.txt'))) {
    const archive = join(cacheDir(), 'downloads', 'hermes', archiveFileName('hermes', pin));
    task.progress('downloading');
    await downloadVerified(pin.url, pin.sha256, archive);
    task.progress('extracting');
    await extractSource(task, archive, paths.source);
  }
  await buildCMakeProject(task, {
    source: paths.source,
    build: paths.build,
    definitions: HOST_DEFINITIONS,
    targets: ['hermesc', 'shermes'],
  });
  return { version: pin.version, importFile: paths.importFile, hermesc: paths.hermesc };
}
