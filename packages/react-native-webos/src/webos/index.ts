export {
  connectDevice,
  listDevices,
  probeDevice,
  resolveDevice,
  type Device,
  type DeviceStatus,
} from './devices.ts';
export {
  deployPackage,
  openPackage,
  signedCopy,
  type DeployRequest,
  type OpenedPackage,
} from './deploy-package.ts';
export { installApp, launchApp, uninstallApp, type InstallRequest } from './install-app.ts';
export { packageInfo, type PackageInfo } from './package-info.ts';
export { addProfile, setActiveProfile, type NewProfile } from './profiles-edit.ts';
export {
  parseProfiles,
  profilesPath,
  readProfiles,
  type ProfileKey,
  type ProfilesFile,
  type SecurityProfile,
} from './profiles-file.ts';
export { profilePassword } from './profile-password.ts';
export { storePassword } from './password-store.ts';
export { resolveSigning, type ResolvedSigning, type SigningRequest } from './signing-identity.ts';
export { readKnownDevices, rememberDevice } from './known-devices.ts';
export { parseDeviceAddress, deviceSerial } from './device-address.ts';

export {
  openSdbDevice,
  withSdbDevice,
  SdbCommandError,
  type InstallProgress,
  type SdbDevice,
} from '../sdb/sdb-device.ts';
export {
  SdbConnection,
  SdbConnectError,
  SDB_PORT,
  type ConnectFailure,
  type SdbAddress,
} from '../sdb/sdb-connection.ts';
export {
  parseCapabilities,
  readCapabilities,
  type DeviceCapabilities,
} from '../sdb/capabilities.ts';
export { runShell, type ShellOptions } from '../sdb/shell-command.ts';
export { pushFile } from '../sdb/sync-push.ts';

export {
  createAuthorCertificate,
  type AuthorCertificate,
  type AuthorRequest,
} from '../signing/author-certificate.ts';
export { distributorDuids } from '../signing/distributor-duids.ts';
export {
  issueCertificate,
  type CertificateRequest,
  type DistinguishedName,
} from '../signing/x509.ts';
export { readPkcs12, Pkcs12PasswordError, type Pkcs12Contents } from '../signing/pkcs12.ts';
export { writePkcs12 } from '../signing/pkcs12-write.ts';
export { signingKeyFrom } from '../signing/signing-key.ts';
export {
  signFiles,
  signPackage,
  packageEntries,
  type SigningIdentity,
} from '../signing/sign-package.ts';
export type { SigningKey, SignatureRole } from '../signing/signature-xml.ts';
export { readZip, type ZipEntry } from '../signing/zip-read.ts';
export { writeZip } from '../signing/zip-write.ts';

export { scanForTvs, type FoundTv, type ScanOptions } from '../discovery/scan-tvs.ts';
export { probeTv, type SdbState, type TvProbe } from '../discovery/probe-tv.ts';
export {
  fetchApiDevice,
  SAMSUNG_API_PORT,
  type SamsungApiDevice,
} from '../discovery/samsung-api.ts';
export { ssdpSearch, type SsdpReply } from '../discovery/ssdp.ts';
