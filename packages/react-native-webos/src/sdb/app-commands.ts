/** Where sdb pushes a package when the TV does not report `sdk_toolpath`. */
export const DEFAULT_TOOL_PATH = '/home/owner/share/tmp/sdk_tools';
export const DUID_COMMAND = '0 getduid';

const PACKAGE_ID = /^[A-Za-z0-9_.]+$/;
const UNSAFE_NAME = /[^A-Za-z0-9._-]/g;
const PROGRESS = /\binstalling\[(\d{1,3})\]/;
const INSTALLED = /\binstall completed\b/;
const UNINSTALLED = /\buninstall completed\b/;
const LAUNCHED = /\bsuccessfully launched\b/;

function checkedId(id: string): string {
  if (!PACKAGE_ID.test(id)) throw new Error(`sdb: refusing to send the id ${JSON.stringify(id)}`);
  return id;
}

/** The path a package is pushed to: sdb's `tmp/` under the TV's tool path, the name made shell-safe. */
export function remotePackagePath(toolPath: string, fileName: string): string {
  const name = fileName.replace(UNSAFE_NAME, '_').replace(/^[._]+/, '') || 'package.tpk';
  const directory = toolPath.endsWith('/') ? toolPath.slice(0, -1) : toolPath;
  return `${directory}/tmp/${name}`;
}

export function installCommand(packageId: string, remotePath: string): string {
  return `0 vd_appinstall ${checkedId(packageId)} ${remotePath}`;
}

export function uninstallCommand(packageId: string): string {
  return `0 appuninstall ${checkedId(packageId)}`;
}

export function launchCommand(appId: string): string {
  return `0 execute ${checkedId(appId)}`;
}

/** The percentage an install output line reports, if it reports one. */
export function installProgress(line: string): number | null {
  const match = PROGRESS.exec(line);
  return match?.[1] === undefined ? null : Number(match[1]);
}

export function installSucceeded(output: string): boolean {
  return INSTALLED.test(output);
}

export function uninstallSucceeded(output: string): boolean {
  return UNINSTALLED.test(output);
}

export function launchSucceeded(output: string): boolean {
  return LAUNCHED.test(output);
}

/** The model a TV names in its banner, `device::<model>::0`. */
export function bannerModel(banner: string): string | null {
  const [kind, model] = banner.split('::');
  return kind === 'device' && model !== undefined && model !== '' ? model : null;
}
