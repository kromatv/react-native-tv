# The `tizen` API

`@kromatv/react-native-tizen/tizen` is what the `rn-tizen` CLI does to a TV, as
a typed Node API: find Samsung TVs, talk sdb to them, sign `.tpk` and `.wgt`
packages, and manage Tizen Studio's signing profiles. It needs Node 22.13 or
newer and nothing from Tizen Studio. Every export is typed; this page lists the
ones a deployment tool needs.

```ts
import {
  deployPackage,
  openPackage,
  probeDevice,
  resolveSigning,
  scanForTvs,
} from '@kromatv/react-native-tizen/tizen';
```

## Find TVs

```ts
const tvs = await scanForTvs({ onProgress: (done, total) => {} });
// [{ host: '192.168.1.50', name: 'Living room', model: 'QE55...', developerMode: true, developerIp: '192.168.1.10' }]

const probe = await probeTv(tvs[0]);
// probe.sdb: { state: 'ready', capabilities, model, duid } or { state: 'refused' | 'closed' | 'silent', reason }
```

`scanForTvs` sends an SSDP search and sweeps the local `/24`s for Samsung's
REST API (port 8001) and sdb (26101). `developerMode` is null when the REST API
did not answer. `probeTv` connects over sdb and reads the CPU, the Tizen
version and the DUID. A TV takes one sdb connection at a time: `refused` means
another computer or Tizen Studio holds it.

## Sign and install

```ts
const device = await probeDevice({ host: '192.168.1.50', port: 26101 });
const signing = await resolveSigning({ profile: 'MyTV' });
const opened = await openPackage('dist/app.wgt');
await deployPackage(task, { package: opened, device, signing, launch: true });
```

- `resolveSigning({ profile?, author?, distributor? })` opens a profile from
  Tizen Studio's `profiles.xml` (the active one by default), or `.p12` files,
  and checks their passwords and expiry. Passwords come from the profile, the
  macOS keychain, or `RN_TIZEN_AUTHOR_PASSWORD` and
  `RN_TIZEN_DISTRIBUTOR_PASSWORD`.
- `openPackage(file)` reads a `.tpk` or `.wgt` and its ids (`info.packageId`,
  `info.appId`, `info.installId`).
- `deployPackage(task, request)` signs a copy for the TV, refuses a TV whose
  DUID the distributor certificate does not list, installs, and launches when
  `launch` is true. `task` reports progress: `{ run, progress(detail) }`, where
  `run` is not used here.
- `signFiles(files, identity)` and `signPackage(path, identity, out?)` sign
  without installing; their output is byte for byte what `tizen package`
  writes.

## The sdb client

```ts
await withSdbDevice({ host: '192.168.1.50', port: 26101 }, async (tv) => {
  tv.capabilities; // { cpuArch, platformVersion, profile, toolPath }
  await tv.install('app.tpk', 'com.example.app', ({ phase, percent }) => {});
  await tv.launch('com.example.app');
  await tv.uninstall('com.example.app');
  return tv.duid();
});
```

`SdbConnection`, `runShell` and `pushFile` are the layers underneath. A failed
connection throws `SdbConnectError` with `reason` `unreachable`, `refused` or
`silent`; a failed install or launch throws `SdbCommandError` with what the TV
printed.

## Certificates and profiles

- `readProfiles(path?)`, `profilesPath()`: Tizen Studio's profiles.
- `addProfile(path, profile)`, `setActiveProfile(path, name)`: edit
  `profiles.xml`, keeping a `.bak` copy.
- `storePassword(pwdPath, password)`: store a certificate password where Tizen
  Studio reads it.
- `readPkcs12(bytes, password)`, `writePkcs12(key, chain, password, alias)`,
  `signingKeyFrom(contents)`: PKCS#12 in TypeScript, RC2 included.
- `createAuthorCertificate({ directory, subject })`: a self-signed author
  certificate. A retail Samsung TV refuses it; Samsung issues the author
  certificate a TV takes along with the distributor one.
- `distributorDuids(certificate)`: the DUIDs a distributor certificate lists.

The facts behind each of these are in [`reference/`](reference/README.md):
`sdb-protocol.md`, `tizen-package-signing.md` and `tv-discovery.md`.
