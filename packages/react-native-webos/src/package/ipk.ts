import { lstat, mkdir, readdir, readFile, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { gzipSync } from 'node:zlib';
import { FixableError } from '../errors.ts';
import { APPINFO_FILE, appInfoSchema } from './appinfo.ts';
import { writeAr } from './ar-archive.ts';
import { ELF_MACHINE_ARM, elfMachine } from './elf-machine.ts';
import { controlFile } from './ipk-control.ts';
import { writeTar, type TarEntry } from './ustar.ts';

/** What {@link writeIpk} is given. */
export interface IpkRequest {
  /** The app directory, `appinfo.json` at its root, as it installs under `usr/palm/applications/<id>`. */
  readonly appDir: string;
  readonly outDir: string;
  /** The time every member and entry is dated with (default: now). */
  readonly now?: Date;
}

/** A package {@link writeIpk} wrote. */
export interface WrittenIpk {
  readonly file: string;
  readonly appId: string;
  readonly version: string;
  /** The bytes the package installs, as its `control` file says. */
  readonly installedSize: number;
}

const ARCHITECTURE = 'arm';
const DIRECTORY_MODE = 0o777;
const EXECUTABLE_MODE = 0o755;
const FILE_MODE = 0o644;
const EXECUTE_BITS = 0o111;
const MAX_APPINFO_BYTES = 1024 * 1024;
const DEBIAN_BINARY = '2.0\n';

async function readAppInfo(appDir: string) {
  const file = join(appDir, APPINFO_FILE);
  const text = await readFile(file, 'utf8').catch(() => null);
  if (text === null) throw new Error(`${appDir} holds no ${APPINFO_FILE}`);
  if (text.length > MAX_APPINFO_BYTES) throw new Error(`${file} is too large`);
  const parsed = appInfoSchema.safeParse(JSON.parse(text));
  if (parsed.success) return parsed.data;
  const issue = parsed.error.issues[0];
  throw new Error(`${file}: ${issue?.path.join('.') ?? ''}: ${issue?.message ?? 'not valid'}`);
}

async function collect(dir: string, path: string, main: string): Promise<TarEntry[]> {
  const children = await readdir(dir, { withFileTypes: true });
  const entries = await Promise.all(
    children.map(async (child): Promise<TarEntry[]> => {
      const childPath = path === '' ? child.name : `${path}/${child.name}`;
      const onDisk = join(dir, child.name);
      if (child.isDirectory()) {
        const below = await collect(onDisk, childPath, main);
        return [{ path: childPath, mode: DIRECTORY_MODE }, ...below];
      }
      if (!child.isFile()) {
        throw new Error(`${onDisk} is neither a file nor a directory: a package holds only those`);
      }
      const executable = childPath === main || ((await lstat(onDisk)).mode & EXECUTE_BITS) !== 0;
      const data = await readFile(onDisk);
      return [{ path: childPath, mode: executable ? EXECUTABLE_MODE : FILE_MODE, data }];
    }),
  );
  return entries.flat();
}

function requireArmExecutable(entries: readonly TarEntry[], main: string): void {
  const executable = entries.find((entry) => entry.path === main)?.data;
  if (executable === undefined) throw new Error(`The app directory holds no ${main}`);
  // Every webOS TV runs apps as 32-bit ARM, and what its opkg does with another
  // Architecture is unknown (docs/reference/webos-ipk-format.md, open questions).
  if (elfMachine(executable) === ELF_MACHINE_ARM) return;
  throw new FixableError(
    `${main} is not a 32-bit ARM executable, the only kind a webOS TV runs`,
    'Build the runtime for the TV with rn-webos build-runtime: a simulator build runs in the simulator only.',
  );
}

function withParents(root: string): TarEntry[] {
  const parts = root.split('/');
  return parts.map((_, index) => ({
    path: parts.slice(0, index + 1).join('/'),
    mode: DIRECTORY_MODE,
  }));
}

function dataEntries(appId: string, version: string, app: readonly TarEntry[]): TarEntry[] {
  const appRoot = `usr/palm/applications/${appId}`;
  const packageRoot = `usr/palm/packages/${appId}`;
  const packageInfo = `${JSON.stringify({ id: appId, version, app: appId }, null, 2)}\n`;
  return [
    ...withParents(appRoot),
    ...app.map((entry) => ({ ...entry, path: `${appRoot}/${entry.path}` })),
    ...withParents(packageRoot).slice(2),
    { path: `${packageRoot}/packageinfo.json`, mode: FILE_MODE, data: Buffer.from(packageInfo) },
  ];
}

/**
 * Writes the `.ipk` a webOS TV installs from an app directory, as LG's
 * `ares-package` does but with fixed modes, owners and order: an `ar` archive
 * of `debian-binary`, `control.tar.gz` and `data.tar.gz`, named
 * `<id>_<version>_arm.ipk`. Nothing is minified or left out.
 */
export async function writeIpk(request: IpkRequest): Promise<WrittenIpk> {
  const info = await readAppInfo(request.appDir);
  const app = await collect(request.appDir, '', info.main);
  requireArmExecutable(app, info.main);
  const data = dataEntries(info.id, info.version, app);
  const installedSize = data.reduce((total, entry) => total + (entry.data?.byteLength ?? 0), 0);
  const control = controlFile({
    packageId: info.id,
    version: info.version,
    architecture: ARCHITECTURE,
    installedSize,
  });
  // A TV refuses a package whose entries are all dated 1970, so every entry
  // carries the time of packaging (docs/reference/webos-ipk-format.md, section 4).
  const mtime = Math.floor((request.now ?? new Date()).getTime() / 1000);
  const controlTar = writeTar(
    [{ path: 'control', mode: FILE_MODE, data: Buffer.from(control) }],
    mtime,
  );
  const archive = writeAr(
    [
      { name: 'debian-binary', data: Buffer.from(DEBIAN_BINARY) },
      { name: 'control.tar.gz', data: gzipSync(controlTar) },
      { name: 'data.tar.gz', data: gzipSync(writeTar(data, mtime)) },
    ],
    mtime,
  );
  const file = join(request.outDir, `${info.id}_${info.version}_${ARCHITECTURE}.ipk`);
  await mkdir(request.outDir, { recursive: true });
  await writeFile(file, archive);
  return { file, appId: info.id, version: info.version, installedSize };
}
