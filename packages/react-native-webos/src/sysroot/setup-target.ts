import { join } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import { RUNTIME_DIR } from '../package-paths.ts';
import { buildCMakeProject, sourcesKey, webosDefinitions } from '../runtime/cmake.ts';
import type { Task } from '../task.ts';
import { locateRootstrap, type WebOSSdk } from '../webos/webos-sdk.ts';
import { rootstrapFor, type WebOSTarget } from '../webos/target.ts';
import { mirrorSysroot, type MirroredSysroot } from './mirror-sysroot.ts';

/**
 * Makes a target buildable: compiles the glibc compat archive against the raw
 * rootstrap, then mirrors the rootstrap into a sysroot lld can link against.
 * Cheap to repeat; an unchanged rootstrap is reused.
 */
export async function setupTarget(
  task: Task,
  sdk: WebOSSdk,
  target: WebOSTarget,
): Promise<MirroredSysroot> {
  const rootstrap = locateRootstrap(sdk, target);
  const { name } = rootstrapFor(target);
  const compatBuild = join(cacheDir(), 'compat', `${name}-${sourcesKey(RUNTIME_DIR)}`);
  await buildCMakeProject(task, {
    source: join(RUNTIME_DIR, 'compat'),
    build: compatBuild,
    definitions: { ...webosDefinitions(sdk, target), WEBOS_USE_RAW_ROOTSTRAP: 'ON' },
  });
  return mirrorSysroot({
    rootstrap,
    destination: join(cacheDir(), 'sysroots', name),
    compatArchive: join(compatBuild, 'librn-webos-glibc-compat.a'),
  });
}
