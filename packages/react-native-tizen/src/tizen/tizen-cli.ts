import { basename, dirname, resolve } from 'node:path';
import { FixableError } from '../errors.ts';
import type { Task } from '../task.ts';
import { parseSecurityProfiles, type SecurityProfile } from './security-profiles.ts';
import type { TizenSdk } from './tizen-sdk.ts';

/** How to get a security profile, for a TV that refuses unsigned packages. */
export const CERTIFICATE_HINT =
  'Create a Samsung certificate for this TV in Tizen Studio (Tools > Certificate Manager), or pass --profile <name>.';

/** The signing profiles Tizen Studio knows. */
export async function listSecurityProfiles(task: Task, sdk: TizenSdk): Promise<SecurityProfile[]> {
  return parseSecurityProfiles(await task.run([sdk.tizenCli, 'security-profiles', 'list']));
}

/** The profile to sign with: `requested` when given, the active one otherwise. */
export async function signingProfile(
  task: Task,
  sdk: TizenSdk,
  requested: string | undefined,
): Promise<string> {
  const profiles = await listSecurityProfiles(task, sdk);
  const names = profiles.map((profile) => profile.name);
  if (requested !== undefined) {
    if (names.includes(requested)) return requested;
    const known = names.length > 0 ? `Known profiles: ${names.join(', ')}.` : CERTIFICATE_HINT;
    throw new FixableError(`No security profile named ${requested}`, known);
  }
  const active = profiles.find((profile) => profile.active);
  if (active !== undefined) return active.name;
  throw new FixableError('No active security profile', CERTIFICATE_HINT);
}

/**
 * Re-signs a package in place. The Tizen CLI exits 0 even when signing did not
 * happen, so success is read from what it printed.
 */
export async function signPackage(
  task: Task,
  sdk: TizenSdk,
  tpk: string,
  profile: string,
): Promise<void> {
  const output = await task.run([
    sdk.tizenCli,
    'package',
    '-t',
    'tpk',
    '-s',
    profile,
    '--',
    resolve(tpk),
  ]);
  if (/author cert/i.test(output) && /created successfully/i.test(output)) return;
  throw new FixableError(`Signing with ${profile} failed`, CERTIFICATE_HINT);
}

/** Installs a package on a TV, replacing any earlier install. */
export async function installPackage(
  task: Task,
  sdk: TizenSdk,
  serial: string,
  tpk: string,
): Promise<void> {
  const output = await task.run([
    sdk.tizenCli,
    'install',
    '-s',
    serial,
    '-n',
    basename(tpk),
    '--',
    dirname(resolve(tpk)),
  ]);
  if (!/fail|error/i.test(output)) return;
  throw new FixableError(
    `Installing on ${serial} failed`,
    `The certificate must list this TV's DUID, and the TV must be in Developer mode. Tizen said:\n${output.trim()}`,
  );
}

/** Launches an installed package. */
export async function launchPackage(
  task: Task,
  sdk: TizenSdk,
  serial: string,
  packageId: string,
): Promise<void> {
  const output = await task.run([sdk.tizenCli, 'run', '-s', serial, '-p', packageId]);
  if (!/fail|error/i.test(output)) return;
  throw new FixableError(`Launching ${packageId} failed`, `Tizen said:\n${output.trim()}`);
}
