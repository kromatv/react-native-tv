import { z } from 'zod';
import { firstTagAttributes } from './xml-attributes.ts';

/** What kind of package a file is, the id it installs under and the app it launches. */
export interface PackageInfo {
  readonly kind: 'tpk' | 'wgt';
  readonly packageId: string;
  readonly appId: string;
  /** The id the TV's install command takes: the package's for a `.tpk`, the app's for a `.wgt`. */
  readonly installId: string;
}

const MAX_MANIFEST_BYTES = 256 * 1024;
const id = z.string().regex(/^[A-Za-z0-9_.]{1,128}$/, 'letters, digits, dots and underscores');

function manifestText(files: ReadonlyMap<string, Buffer>, name: string): string | null {
  const bytes = files.get(name);
  if (bytes === undefined) return null;
  if (bytes.length > MAX_MANIFEST_BYTES) throw new Error(`${name} is too large to be a manifest`);
  return bytes.toString('utf8');
}

function tpkInfo(xml: string): PackageInfo {
  const manifest = firstTagAttributes(xml, 'manifest');
  const app =
    firstTagAttributes(xml, 'ui-application') ?? firstTagAttributes(xml, 'service-application');
  const packageId = id.parse(manifest?.package);
  return { kind: 'tpk', packageId, appId: id.parse(app?.appid), installId: packageId };
}

function wgtInfo(xml: string): PackageInfo {
  const application = firstTagAttributes(xml, 'tizen:application');
  const appId = id.parse(application?.id);
  return { kind: 'wgt', packageId: id.parse(application?.package), appId, installId: appId };
}

/** Reads a package's ids from its `tizen-manifest.xml` (a `.tpk`) or its `config.xml` (a `.wgt`). */
export function packageInfo(files: ReadonlyMap<string, Buffer>): PackageInfo {
  const manifest = manifestText(files, 'tizen-manifest.xml');
  if (manifest !== null) return tpkInfo(manifest);
  const config = manifestText(files, 'config.xml');
  if (config !== null) return wgtInfo(config);
  throw new Error('The package has neither a tizen-manifest.xml nor a config.xml at its root');
}
