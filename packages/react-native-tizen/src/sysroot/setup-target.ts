import { join } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import { RUNTIME_DIR } from '../package-paths.ts';
import { buildCMakeProject, sourcesKey, tizenDefinitions } from '../runtime/cmake.ts';
import type { Task } from '../task.ts';
import { locateRootstrap, type TizenSdk } from '../tizen/tizen-sdk.ts';
import { rootstrapName, type TizenTarget } from '../tizen/target.ts';
import { mirrorSysroot, type MirroredSysroot } from './mirror-sysroot.ts';

/**
 * Makes a target buildable: compiles the glibc compat archive against the raw
 * rootstrap, then mirrors the rootstrap into a sysroot lld can link against.
 * Cheap to repeat; an unchanged rootstrap is reused.
 */
export async function setupTarget(
  task: Task,
  sdk: TizenSdk,
  target: TizenTarget,
): Promise<MirroredSysroot> {
  const rootstrap = locateRootstrap(sdk, target);
  const name = rootstrapName(target);
  const compatBuild = join(cacheDir(), 'compat', `${name}-${sourcesKey(RUNTIME_DIR)}`);
  await buildCMakeProject(task, {
    source: join(RUNTIME_DIR, 'compat'),
    build: compatBuild,
    definitions: { ...tizenDefinitions(sdk, target), TIZEN_USE_RAW_ROOTSTRAP: 'ON' },
  });
  return mirrorSysroot({
    rootstrap,
    destination: join(cacheDir(), 'sysroots', name),
    compatArchive: join(compatBuild, 'librn-tizen-glibc-compat.a'),
  });
}
