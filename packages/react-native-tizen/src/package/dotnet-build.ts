import { existsSync } from 'node:fs';
import { rm } from 'node:fs/promises';
import { join } from 'node:path';
import type { TizenAppConfig } from '../config/tizen-config.ts';
import type { Task } from '../task.ts';
import type { PlatformProfile } from '../tizen/platform-profiles.ts';

function packagePath(projectDir: string, config: TizenAppConfig, profile: PlatformProfile): string {
  const name = `${config.packageId}-${config.version}.tpk`;
  return join(projectDir, 'bin', 'Release', profile.targetFramework, name);
}

/**
 * Builds the .NET shell into a `.tpk`. Tizen.NET signs it with its default
 * certificate, which no retail TV accepts, so it still has to be re-signed.
 */
export async function buildDotnetProject(
  task: Task,
  projectDir: string,
  config: TizenAppConfig,
  profile: PlatformProfile,
): Promise<string> {
  const tpk = packagePath(projectDir, config, profile);
  await rm(tpk, { force: true });
  await task.run(['dotnet', 'build', '-c', 'Release', '-nologo'], { cwd: projectDir });
  if (!existsSync(tpk)) throw new Error(`dotnet build reported success but wrote no ${tpk}`);
  return tpk;
}
