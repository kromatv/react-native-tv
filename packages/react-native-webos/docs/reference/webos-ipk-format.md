# The `.ipk` a webOS TV installs

The package LG's `ares-package` produces, member by member, precise enough to
write it from TypeScript, and what the installer is known to read of it.
Checked on 2026-11-11.

**Nothing in this file was checked on a TV by this project.** No package
described here was installed anywhere.

| Tag | Meaning |
|---|---|
| **D** | Documented: a page or a file somebody publishes. The source column says who. Another project's report from its own TV is still only documentation here |
| **S** | Read from source: file and line |
| **I** | Inferred, and the row says from what. **I (probe)** means: from packages built here with `ares-package` 3.2.5 and taken apart byte by byte |

How sources are written:

- `cli:` is a path inside the npm package `@webos-tools/cli` 3.2.5.
- `tar:` is a path inside the npm package `tar` 7.5.13, `elfy:` inside `elfy`
  1.0.0: the versions `cli:package.json` pins.
- "OSE" marks the open-source installer of webOS OSE. The TV's installer is
  not public; the OSE one is its closest published relative.
- Everything else is a link in [Sources](#sources), followed by line numbers.

The probe: `ares-package` 3.2.5 under Node 26.5.0 on macOS (APFS), run on a
throwaway app: an `appinfo.json` with `"type": "native"` and `"main":
"bin/probe"`; `bin/probe`, 128 bytes starting with an ARM ELF header, mode
0755; `lib/libfoo.so`, mode 0644; `icon.png`; `assets/main.js`; the directory
`assets` at mode 0750. Variants added a 122-byte path, a `main` of mode 0644, a
`.js` file that is not JavaScript, `.DS_Store` files, an old `.ipk`, a
`package.properties` and a symlink.

## 1. The `ar` container

| Fact | Tag | Source |
|---|---|---|
| The file starts with the 8 bytes `!<arch>\n`. | S | cli:lib/package.js:1672 |
| Three members, in this order: `debian-binary`, `control.tar.gz`, `data.tar.gz`. | S | cli:lib/package.js:1673-1691 |
| Each member has a 60-byte ASCII header, every field padded with spaces: name (16 bytes, no trailing `/`), modification time (12, decimal seconds, the moment of packaging), uid (6, `0`), gid (6, `0`), mode (8, `100644`), size (10, decimal bytes), then the two bytes `0x60 0x0a`. | S | cli:lib/package.js:1641-1656 |
| Data follows its header. A member of odd size is followed by one `\n` (0x0a). | S | cli:lib/package.js:1685-1690 |
| `debian-binary` holds the 4 bytes `2.0\n`. | S | cli:lib/package.js:1673; 1555-1559 |
| In a probe package of 1,098 bytes: global header at 0; `debian-binary` header at 8; `control.tar.gz` header at 72 (288 bytes of data); `data.tar.gz` header at 420 (617 bytes), then one pad byte. All three headers carried the same second. `file` calls it "Debian binary package (format 2.0)". | I (probe) | |
| GNU `ar` ends short member names with `/`. A TV refused such a package with `error_code -5`, "Failed to extract package"; the same content with bare names installed. | D | [plx] 1488-1512 (webOS 4.x TV) |
| The OSE installer runs `ar x <ipk> control.tar.gz` in a temporary directory, then `tar xzf control.tar.gz`, and reads the file `control`. It asks for members by these names: `control.tar.gz`, `data.tar.gz`, `debian-binary`. | S | [ai2-package] 27-29, 79-88, 152-155 (OSE) |
| ares-cli-rs writes the same three members with the Rust `ar` crate: mode `100644`, one timestamp for all. | S | [acr-header] 14-19; [acr-control] 39-42; [acr-data] 57-60 |

## 2. `control.tar.gz` and the `control` file

`control.tar.gz` holds one file, `control` (S: cli:lib/package.js:1285-1291).
Signing files join it only with `--sign` and `--certificate`
(cli:lib/package.js:1517-1553). The `control` file is ten lines, each ended by
`\n`, with no empty line after the last (S: cli:lib/package.js:1447-1475; the
probe's file was 255 bytes):

```
Package: com.example.probe
Version: 1.2.3
Section: misc
Priority: optional
Architecture: arm
Installed-Size: 1559
Maintainer: N/A <nobody@example.com>
Description: This is a webOS application.
webOS-Package-Format-Version: 2
webOS-Packager-Version: x.y.x
```

| Field | Value | Tag | Source |
|---|---|---|---|
| `Package` | the app's `id`, unless `--pkgid` or a `packageinfo.json` given with `--pkginfofile` names another package id | S | cli:lib/package.js:1306-1324 |
| `Version` | the app's `version`. `--pkgversion` is refused when an app is packaged | S | cli:lib/package.js:561, 1306-1324; cli:bin/ares-package.js:264-266 |
| `Section` | `misc`, constant | S | cli:lib/package.js:1450 |
| `Priority` | `optional`, constant | S | cli:lib/package.js:1451 |
| `Architecture` | `all`, unless the file `main` points to starts with `\x7fELF`. Then its `e_machine`: ARM gives `arm`, 386 gives `i586`, x86-64 gives `x86_64`, AArch64 gives `aarch64`, anything else the name `elfy` has for it. Only the first 64 bytes of that one file are read; libraries are not looked at | S | cli:lib/package.js:920-982, 1452; elfy:lib/elfy/constants.js:50-147 |
| `Installed-Size` | bytes, not Debian's kibibytes: the sum of `lstat().size` over every file and every directory of the staged tree, or `1234` when that sum is 0. In the probe 1559 = 439 bytes of files + 1120 bytes that APFS reports for ten directories, so the number depends on the packaging host's file system | S, I (probe) | cli:lib/package.js:1326-1364, 1453 |
| `Maintainer` | `N/A <nobody@example.com>`, constant | S | cli:lib/package.js:1454 |
| `Description` | `This is a webOS application.`, constant | S | cli:lib/package.js:1455 |
| `webOS-Package-Format-Version` | `2`, constant | S | cli:lib/package.js:1456 |
| `webOS-Packager-Version` | `x.y.x`, literally | S | cli:lib/package.js:1457 |

| Fact | Tag | Source |
|---|---|---|
| The OSE installer reads four fields: `Package`, `Version`, `Architecture`, `Installed-Size`. It splits each line at the first `:` and trims the value. | S | [ai2-package] 182-213 (OSE) |
| It takes `Installed-Size` as bytes: it compares it with `f_bavail * f_bsize` of the install partition and fails with "There is no available space to install App". Without the field there is no check. | S | [ai2-parse] 132-141; [ai2-install] 159-199 (OSE) |
| A TV reported the number back as `"unpackFileSize":"432026"` in its install status. | D | [hearth-4] (OLED55CX5LB, webOS 5.6.2) |
| `Package` becomes the package id the rest of the install uses: the OSE installer then loads `usr/palm/applications/<Package>/appinfo.json` and fails with "Cannot find appinfo.json" without it. | S | [ai2-parse] 121; [ai2-svcutil] 74-80 (OSE) |
| `Version` equal to the installed one makes the OSE installer pass `--force-reinstall` to opkg. | S | [ai2-parse] 123-130; [aiu-main] 516-517 (OSE) |
| ares-cli-rs writes the same ten lines. Its `Installed-Size` is also a byte sum: the size `stat` reports for every entry of the app and service directories, plus the length of `packageinfo.json`. | S | [acr-control] 46-62; [acr-input] 90-118; [acr-inputmod] 31-42 |
| webosbrew's verifier calls a package without `Installed-Size`, `webOS-Package-Format-Version` or `webOS-Packager-Version` "hand-rolled" and warns; it ignores an `Installed-Size` of `1234`. Its repository turned a package away for missing them. | S, D | [wb-ipk] 52-58, 144-151; [apps-218] |
| The same verifier: Debian maintainer scripts (`preinst`, `postinst`, `prerm`, `postrm`) are run by no webOS installer. ares writes none. | S | [wb-ipk] 153-155 |

## 3. `data.tar.gz`: the tree

```
usr/
usr/palm/
usr/palm/applications/
usr/palm/applications/<app id>/               the app directory, as given
usr/palm/applications/<app id>/appinfo.json
usr/palm/packages/
usr/palm/packages/<package id>/
usr/palm/packages/<package id>/packageinfo.json
usr/palm/services/<service id>/               one per service, when there are any
```

| Fact | Tag | Source |
|---|---|---|
| Paths have no leading `/` or `./`. Directory entries end in `/`. | I (probe) | |
| The app directory is copied to `usr/palm/applications/<appinfo id>/`. | S | cli:lib/package.js:742-754, 984-992 |
| `packageinfo.json` is generated: `JSON.stringify({id, version, app}, null, 2)` plus `\n`, with `id` the package id, `version` the package version, `app` the app's id. With services, a fourth key `services` lists their ids. In the probe: 84 bytes, keys in that order. | S, I (probe) | cli:lib/package.js:1164-1195, 1940-1973 |
| A service is a directory with a `services.json`. It goes to `usr/palm/services/<its id>/` and is removed from the app directory. Its id must start with `<package id>.`. | S | cli:lib/package.js:1873-1917, 1976-2016 |
| The OSE installer reads `packageinfo.json` only for `services` (and `id`, `version` when there are services). Without the file its code goes on with an empty list of services. | S | [ai2-pkginfo] 34-69; [ai2-svcutil] 83-87 (OSE) |
| webosbrew's verifier opens `usr/palm/packages/<Package>/packageinfo.json` first and needs its `app`; without the file it reports the whole package as unreadable. | S, D | [wb-ipk] 96-116; [plx] 1473-1484 |
| On one TV every installed app, from the store or from the Homebrew Channel, had its `usr/palm/packages/<id>/` entry. | D | [plx] 1476-1478 (webOS 4.x TV) |
| A Developer Mode install unpacks under `/media/developer/apps`, so the app ends up in `/media/developer/apps/usr/palm/applications/<app id>/`. | S | [aiu-main] 65, 498-513; [ai2-settings] 30-38 (OSE) |

## 4. The tar archives

Both archives come from `tar.c({ file, gzip: true, cwd, filter }, names)` of
node-tar, where `names` is the top-level listing of the staged directory (S:
cli:lib/package.js:1597-1609).

| Fact | Tag | Source |
|---|---|---|
| Headers are ustar: magic `ustar\0`, version `00`. | S, I (probe) | tar:dist/commonjs/header.js:185 |
| node-tar is not in portable mode, so each header carries the staged file's uid and gid (501 and 20 in the probe, the packaging user), `uname` = `$USER` when the file is the process owner's, an empty `gname`, and the staged file's mtime. | S, I (probe) | tar:dist/commonjs/write-entry.js:219-239 |
| Every entry in the probe had the same mtime, the second of packaging: staging copies files without keeping their times. | I (probe) | |
| node-tar also writes atime and ctime, 12 bytes each at offsets 476 and 488, inside what POSIX gives to `prefix`. That leaves 130 bytes for `prefix`. | S, I (probe) | tar:dist/commonjs/header.js:171-204 |
| A path under 100 bytes goes into `name`. A longer one is split at a `/` into `prefix` and `name`. A PAX `x` header is written only when no split fits. The probe's 122-byte path was split, with no PAX header anywhere. | S, I (probe) | tar:dist/commonjs/header.js:240-275; tar:dist/commonjs/write-entry.js:240-257 |
| A file or directory name with a non-ASCII character aborts packaging: "Please use the file name in english letters". The code's reason: "opkg does not support Posix Tar fully". | S | cli:lib/package.js:1567-1573 |
| ares sets the process umask to 0 and builds the tree with `mkdirp`, so every directory entry has mode 0777, whatever the source directory had (0750 in the probe). | S, I (probe) | cli:lib/package.js:263, 2144-2147; [acr-commit] |
| A copied file keeps its source mode, with "other read" added (`mode \|= 4`). Probe: 0755 stayed 0755, 0644 stayed 0644. | S, I (probe) | cli:lib/package.js:901, 1582-1585 |
| Files ares writes itself come out 0666: `packageinfo.json`, `control`, and every minified `.js`. | I (probe) | |
| A uid or gid above `0o7777777` is written as 0. | S | cli:lib/package.js:1586-1592 |
| Every directory has its own entry, typeflag `5`, before anything inside it. Files have typeflag `0`. The probe's order was level by level: `usr/`, `usr/palm/`, its two children, their children, and so on. Within a directory the order is the host's `readdir` order; ares sorts nothing. | I (probe) | |
| A symlink that points inside the app directory is kept as a symlink with its relative target (typeflag `2`, mode 0777 in the probe). One that points outside is replaced by a copy of its target. | S, I (probe) | cli:lib/package.js:811-834, 875-884 |
| The archive ends with two 512-byte zero blocks and no further padding (the probe's data tar: 23 blocks, 11,776 bytes). | I (probe) | |
| ares-cli-rs writes a different tar: GNU headers, GNU long names for long paths, uid 0, gid 5000, directories `040777` with the packaging time, files with their source mode and mtime, entries sorted by name, `packageinfo.json` last with mode 0644. | S | [acr-data] 34-54, 88-99, 132-134, 149-170, 185-195 |
| Its authors on what the TV does with these fields: "the install discards both" uid and gid, and "libopkg masks & 0o7777", so the type bits in `mode` do not matter. | D | [acr-commit] |
| The same authors' verifier fails a package in which every data entry has mtime 0: "TVs refuse to install a package like this", "it refuses per-TV". | S | [wb-ipk-verify] 241-263; [wb-ipk-fixtures] |
| On one TV an installed app's directory was `root:root 775`. | D | [plx] 518-520 (49SM9000PLA, webOS 4.5) |

## 5. gzip

| Fact | Tag | Source |
|---|---|---|
| `gzip: true` makes node-tar create `new zlib.Gzip({})`: zlib's default level and strategy. | S | tar:dist/commonjs/pack.js:152-159 |
| Both members of the probe started `1f 8b 08 00 00 00 00 00 00 13`: deflate, no flags, so no file name, mtime 0, and the OS byte 0x13 of a macOS zlib. `zlib.gzipSync(<the tar>)` with no options gave the same bytes as each member. | I (probe) | |
| The OSE installer unpacks with `tar xzf`. | S | [ai2-package] 152-155 (OSE) |
| ares-cli-rs uses `flate2` at `Compression::default()`. | S | [acr-control] 28; [acr-data] 31 |

## 6. The file name

| Fact | Tag | Source |
|---|---|---|
| `<Package>_<Version>_<arch>.ipk`, where `<arch>` is the `Architecture` value, except that `i586` is written `x86`. The probe: `com.example.probe_1.2.3_arm.ipk`. LG's web app examples: `com.domain.app_0.0.1_all.ipk`. | S, I (probe), D | cli:lib/package.js:1612-1624; [lg-cli] |
| An existing file of that name in the output directory is deleted first. | S | cli:lib/package.js:1626-1639 |
| The installer needs the path to end in `.ipk` (see [webos-device-protocol.md](webos-device-protocol.md), section 5). | S | [ai2-service] 311-329 (OSE) |

## 7. What `ares-package` checks first

| Check | Tag | Source |
|---|---|---|
| A directory with `appinfo.json` is the app; there may be one. A directory with `packageinfo.json` is refused. | S | cli:lib/package.js:1733-1752 |
| `appinfo.json` is read as UTF-8, a BOM is stripped, and the file is written back in place, in the source directory. | S | cli:lib/package.js:551-552, 2156-2176 |
| A missing `version` becomes `1.0.0`. | S | cli:lib/package.js:557-559 |
| `id` is required and must match `/^[a-z0-9.+-]*$/`. With a package id given, it must start with it. | S | cli:lib/package.js:588-598 |
| `version` must match `/^([1-9]\d{0,8}\|\d)\.([1-9]\d{0,8}\|\d)\.([1-9]\d{0,8}\|\d)$/`: three numbers of up to nine digits, no leading zero. | S | cli:lib/package.js:599-605 |
| The file is validated against a JSON schema that requires `id`, `main`, `icon`, `type`, `title` and allows `type` to be `web`, `stub`, `native`, `native_builtin`, `native_appshell` or `qml`. | S | cli:lib/package.js:610-663; cli:files/schema/ApplicationDescription.schema:67-78, 312-318 |
| The file `main` names must exist, for every type. | S | cli:lib/package.js:920-928 |
| `main`, `icon`, `largeIcon`, `bgImage`, `splashBackground`, `imageForRecents` and `sysAssetsBasePath`, when present, must be relative paths inside the app directory to something that exists (a name starting with `$` is exempt). | S | cli:lib/package.js:41-49, 666-679, 1016-1038 |
| Image sizes and formats are not checked. | S | the same lines |
| The package id and version are checked with the same two patterns. | S | cli:lib/package.js:1197-1207 |

What LG's page asks beyond that (D, [lg-appinfo]): an id made "only with
lowercase letters (a-z), digits (0-9), minus signs, and periods", starting with
an alphanumeric character, at least two characters long, and not starting with
`com.palm`, `com.webos`, `com.lge` or `com.palmdts`; `icon` "80x80 pixels in
PNG format", `largeIcon` 130x130. The same page says of `type`: "Only `web` is
allowed currently." LG documents native apps for webOS OSE only: `type` is
`native` and `main` is the "executable file name" (D, [ose-appinfo]).

## 8. What `ares-package` changes or leaves out

| Fact | Tag | Source |
|---|---|---|
| By default every file ending in `.js` whose path does not contain `node_modules` is passed through terser 4.8.1's `minify()` and the result written in its place. `-n`, `--no-minify` turns that off. | S, D | cli:lib/package.js:888-899; cli:bin/ares-package.js:47, 68, 173-177; [lg-cli] |
| A `.js` file terser cannot parse aborts packaging: "Failed to minify code. Please check the source code". The probe's `.js` file holding bytes that are not JavaScript did exactly that; with `--no-minify` it was packaged unchanged. | S, I (probe) | cli:lib/package.js:896-899 |
| Left out: files ending in `.ipk` and `package.properties`, plus `--app-exclude` patterns and the `exclude` array of `appinfo.json`. The four metadata files cannot be excluded. | S, I (probe) | cli:lib/package.js:1063-1074, 1103-1151, 1250-1252 |
| `.DS_Store` is on the exclusion list but the pattern is anchored to the start of the full path and never matches: the probe's `.DS_Store` files were packaged. | S, I (probe) | cli:lib/package.js:1068, 1111-1115 |
| `package.properties` lines such as `filemode.755=name` are parsed and then used by nothing in 3.2.5: the probe's file stayed 0644. | S, I (probe) | cli:lib/package.js:1211-1255; cli:lib/tar-filter-pack.js:36-38 (not loaded: cli:lib/package.js:29) |
| Nothing else is transformed: other files are copied byte for byte. | S | cli:lib/package.js:885-906 |

For a native React Native app only the first two rows matter: a JavaScript
bundle or a Hermes bytecode file named `*.js` is rewritten or stops the build
unless minification is off.

## 9. A native executable

| Fact | Tag | Source |
|---|---|---|
| A native app is `"type": "native"` with `main` naming the executable, relative to the app directory. webosbrew's CMake helper writes exactly `type=native main=<target name>` and runs `ares-package <dir>` with no options. | D, S | [ose-appinfo]; [wb-cmake] 68-79 |
| ares uses `main` for two things: the existence check, and the ELF header that decides `Architecture` and the file name. It does this whatever `type` says. | S | cli:lib/package.js:920-982 |
| ares never sets the executable bit. The mode in the archive is the mode on disk: 0755 gives 0755, 0644 gives 0644. | S, I (probe) | cli:lib/package.js:901, 1582-1585 |
| ares-cli-rs reads the ELF header only for `type` `native` (and for services whose `engine` is `native`): ARM gives `arm`, 386 gives `x86`, anything else is an error unless `--force-arch` is given; mixed architectures are refused. File modes come from the source tree. | S | [acr-validation] 32-40, 80-99; [acr-input] 90-118; [acr-data] 163-169 |
| LG's comment next to the `i586` mapping: "current emulator opkg is allowing only all, noarch and i586". | S | cli:lib/package.js:961-964 |
| webosbrew's helper takes the name `<id>_<version>_$ARCH.ipk` from the environment variable `ARCH` and declares that file as what `ares-package` leaves behind. For that to hold with an ARM executable, `ARCH` has to be `arm`. | S, I | [wb-cmake] 15-17, 49-50, 79-81; from section 6 |
| `e_machine` 40 is the value `elfy` names `arm`, and 183 the one it names `AArch64`. | S | elfy:lib/elfy/constants.js:91, 147 |
| An empty directory in the app directory is packaged: a directory entry of mode 0777, as every other directory. | I (probe) | a second probe on 2026-11-11: the fixture of `src/package/ipk.fixtures.ts`, whose `appdata` is empty |
| On that fixture, and on the example app's staged directory with a clang-built ARM executable as `main`, `ares-package --no-minify` and `rn-webos`'s own writer give the same `ar` members, the same `control` fields but `Installed-Size`, and the same tree with the same types, modes and bytes, `packageinfo.json`'s mode apart (0666 from ares, 0644 from `rn-webos`). | I (probe) | `src/package/ipk-ares.test.ts`; the example's package had 23 entries |

## 10. What the installer does with the package

From the OSE sources, in order (S):

1. `ar x <ipk> control.tar.gz`, `tar xzf control.tar.gz`, parse `control`
   ([ai2-package] 79-88, 152-155, 182-213).
2. Close the app if it runs, then check free space against `Installed-Size`
   ([ai2-conf] 2-30; [ai2-install] 43-53).
3. Run `opkg -o /media/developer/apps install <ipk> -f <opkg.conf>`, with
   `OPKG_USE_VFORK=yes` in the environment and `--force-reinstall` or
   `--force-downgrade` when allowed ([aiu-main] 128-133, 498-530). The opkg
   configuration it passes has three lines: `info_dir`, `status_file`,
   `lock_file` ([ai2-opkgconf] 1-3). `--add-arch` options are added only when
   `/etc/opkg/arch.conf` names a `qemux86` architecture, which is an emulator
   image ([aiu-main] 76, 479-509).
4. Read `usr/palm/applications/<Package>/appinfo.json` and, for services,
   `usr/palm/packages/<Package>/packageinfo.json`; generate the bus role and
   permission files for the app ([ai2-svcutil] 68-101).

What is known to be needed, against what ares merely writes:

| Property | ares | Evidence that it matters |
|---|---|---|
| bare `ar` member names | yes | a TV refused names ending in `/` (D, [plx]); OSE extracts by name (S) |
| member order | `debian-binary`, `control.tar.gz`, `data.tar.gz` | none found; OSE extracts by name |
| `ar` time, uid, gid, mode | now, 0, 0, `100644` | none found |
| `Package` | app id | names the app directory the installer reads (S, OSE) |
| `Version` | app version | compared with the installed one (S, OSE) |
| `Architecture` | `arm` or `all` | parsed by OSE's installer and not used there; opkg may check it, and how the TV's opkg is configured is not public |
| `Installed-Size` | bytes | free space check (S, OSE); verifier warning (S) |
| the other six fields | constants | verifier warning for the two `webOS-*` fields (S); none found for the rest |
| `usr/palm/packages/<id>/packageinfo.json` | yes | needed for services (S, OSE); the verifier needs it (S); every app on one TV had it (D) |
| tar uid, gid | the packaging user's | "the install discards both" (D, [acr-commit]) |
| directory mode 0777 | yes | none found; ares-cli-rs copied it to match |
| mtime other than 0 | packaging time | "TVs refuse" all-epoch packages (S, [wb-ipk-verify]) |
| ustar with `prefix`, atime, ctime | yes | none found; ares-cli-rs uses GNU headers instead |
| gzip details | zlib defaults | none found |

## Open questions

- Whether the TV's opkg checks `Architecture`, which values it accepts, and
  whether `aarch64` is ever right for a TV.
- Whether the TV starts a `main` that lacks the executable bit, and what
  mode ares gives it when it packages on Windows.
- What owner and mode installed files get, and whether a tar uid or gid ever
  reaches the TV's file system.
- Whether the TV's `tar` accepts PAX headers. ustar `prefix` (ares) and GNU
  long names (ares-cli-rs) are what the two packagers write; ares wrote no
  PAX header in the probe.
- Which firmware refuses a package whose entries all have mtime 0, and
  whether one real timestamp is enough.
- Whether the TV compares `Installed-Size` with free space as OSE does, and
  what it does when the field is far too small.
- Whether `packageinfo.json` is required on a TV for an app without services.
- Whether a TV accepts `control.tar.xz` or `data.tar.xz`. ares cannot read
  them (cli:lib/package.js:378-379, 394-395).
- Whether the order of `ar` members or of tar entries matters to any TV.

## What this means for react-native-webos

- The format needs no LG tool: an `ar` writer is 60-byte headers and a pad
  byte, and `node:zlib` already produces the gzip stream ares produces.
- Write bare member names in ares' order, a real timestamp everywhere, and
  all ten `control` lines exactly as ares does, so that neither a TV nor
  webosbrew's verifier sees a "hand-rolled" package.
- `Package` and the directory under `usr/palm/applications/` must be the app
  id; always write `usr/palm/packages/<id>/packageinfo.json` with `id`,
  `version` and `app`.
- `Installed-Size` is bytes. Sum the file sizes; do not copy ares' directory
  sizes.
- `Architecture: arm` and `_arm.ipk` for a native 32-bit ARM app. Read
  `e_machine` from `main` as both packagers do, and fail on anything else
  until the first open question is answered.
- Set modes ourselves instead of trusting the working tree: 0755 for `main`
  and other executables, 0644 for files, 0777 for directories as both
  packagers write. That takes the packaging host's file system out of the
  result.
- Plain ustar with uid 0 and gid 0 is the smallest tar that stays inside what
  ares emits. Keep paths ASCII, split long paths into `prefix` and `name`,
  refuse a path component over 100 bytes instead of writing PAX, and sort
  entries so that two builds of the same tree differ only in timestamps.
- Never minify. The bundle goes into the package as built.
- Validate `appinfo.json` with the two patterns of section 7 and LG's
  reserved prefixes before packaging, and name the file
  `<id>_<version>_arm.ipk`.

## Sources

[lg-cli]: https://webostv.developer.lge.com/develop/tools/cli-dev-guide
[lg-appinfo]: https://webostv.developer.lge.com/develop/references/appinfo-json
[ose-appinfo]: https://www.webosose.org/docs/guides/development/configuration-files/appinfo-json/
[ai2-package]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/installer/AppPackage.cpp
[ai2-parse]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/step/IpkParseStep.cpp
[ai2-install]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/step/IpkInstallStep.cpp
[ai2-service]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/service/AppInstallService.cpp
[ai2-settings]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/settings/Settings.cpp
[ai2-svcutil]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/installer/ServiceInstallerUtility.cpp
[ai2-pkginfo]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/installer/PackageInfo.cpp
[ai2-opkgconf]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/files/opkg/opkg.conf
[ai2-conf]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/files/conf/appinstalld-conf.json.in
[aiu-main]: https://github.com/webosose/applicationinstallerutility/blob/eb1cddc222cf8c64bba52ef9ed597fc6f2c99d49/src/Main.cpp.in
[acr-header]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-package/src/packaging/header.rs
[acr-control]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-package/src/packaging/control.rs
[acr-data]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-package/src/packaging/data.rs
[acr-validation]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-package/src/input/validation.rs
[acr-input]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-package/src/input/data.rs
[acr-inputmod]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-package/src/input/mod.rs
[acr-commit]: https://github.com/webosbrew/ares-cli-rs/commit/17537783f4
[wb-ipk]: https://github.com/webosbrew/dev-toolbox-cli/blob/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/common/ipk/src/ipk.rs
[wb-ipk-verify]: https://github.com/webosbrew/dev-toolbox-cli/blob/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/packages/ipk-verify/src/main.rs
[wb-ipk-fixtures]: https://github.com/webosbrew/dev-toolbox-cli/blob/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/common/ipk/src/fixtures/README.md
[wb-cmake]: https://github.com/webosbrew/dev-utils/blob/b25ad97e737cb22181a8d962c10966c6a5327810/cmake/WebOSPackage.cmake
[apps-218]: https://github.com/webosbrew/apps-repo/issues/218
[plx]: https://github.com/GLinnik21/plx-native/blob/f8f6dbb75c783de7c790608653e212a14c2ec66c/docs/distribution.md
[hearth-4]: https://github.com/rudra-avatar/Hearth/issues/4
