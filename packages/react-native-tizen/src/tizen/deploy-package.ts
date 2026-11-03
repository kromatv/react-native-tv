import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { basename, join } from 'node:path';
import { distributorDuids } from '../signing/distributor-duids.ts';
import { packageEntries, signFiles, type SigningIdentity } from '../signing/sign-package.ts';
import { readZip } from '../signing/zip-read.ts';
import { writeZip } from '../signing/zip-write.ts';
import type { Device } from './devices.ts';
import { installApp, launchApp } from './install-app.ts';
import { packageInfo, type PackageInfo } from './package-info.ts';
import type { ResolvedSigning } from './signing-identity.ts';

/** A `.tpk` or `.wgt` read into memory, with the ids its manifest names. */
export interface OpenedPackage {
  readonly file: string;
  readonly files: Map<string, Buffer>;
  readonly info: PackageInfo;
}

export async function openPackage(file: string): Promise<OpenedPackage> {
  const files = new Map<string, Buffer>();
  for (const entry of readZip(await readFile(file)))
    if (!entry.directory) files.set(entry.name, entry.data);
  return { file, files, info: packageInfo(files) };
}

/** Signs a copy of the package in a new temporary directory, which the caller removes. */
export async function signedCopy(
  opened: OpenedPackage,
  identity: SigningIdentity,
): Promise<string> {
  const files = new Map(opened.files);
  signFiles(files, identity);
  const directory = await mkdtemp(join(tmpdir(), 'rn-tizen-package-'));
  const out = join(directory, basename(opened.file));
  await writeFile(out, writeZip(packageEntries(files)));
  return out;
}

export interface DeployRequest {
  readonly package: OpenedPackage;
  readonly device: Device;
  /** The certificates to sign it with; null installs it as it is signed. */
  readonly signing: ResolvedSigning | null;
  readonly launch: boolean;
  /** Called with what the install is doing, such as `sending 40%`. */
  readonly progress?: (detail: string) => void;
}

/** Signs a package for the TV, installs it, and launches its app when asked. */
export async function deployPackage(request: DeployRequest): Promise<void> {
  const { info } = request.package;
  const signed =
    request.signing === null ? null : await signedCopy(request.package, request.signing.identity);
  try {
    await installApp({ progress: request.progress ?? (() => undefined) }, request.device, {
      file: signed ?? request.package.file,
      installId: info.installId,
      duids: request.signing === null ? [] : distributorDuids(request.signing.distributor),
    });
  } finally {
    if (signed !== null) await rm(join(signed, '..'), { recursive: true, force: true });
  }
  if (request.launch) await launchApp(request.device, info.appId);
}
