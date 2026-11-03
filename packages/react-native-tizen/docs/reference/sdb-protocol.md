# sdb: the wire protocol to a TV

What `rn-tizen` needs to reach a Samsung TV in Developer mode without Tizen
Studio's `sdb`: the packets, the services a TV answers, and what an install and
a launch send. Checked on 2026-11-03.

| Tag | Meaning |
|---|---|
| **D** | Documented: adb's protocol notes, Tizen's or Samsung's pages |
| **H** | Seen on hardware: GQ75LS03DAUXZG, 2024 Frame, Tizen 9.0, armv7, Developer mode. Traffic was recorded by a logging TCP proxy between the client and the TV, the client being Tizen Studio 10.0's `sdb` 4.2.36 (driven by `sdb` and `tizen install`/`tizen run`) or this package's own |
| **I** | Inferred from the rows it cites |

## 1. Where the TV listens

| Fact | Tag | Source |
|---|---|---|
| sdb is a client and a server on the computer and a daemon on the device; the server "manages communication between the client and the daemon". | D | [td-sdb] |
| `sdb connect <host>[:<port>]` reaches a device over TCP, port 26101 by default. | D | [td-sdb], [sam-device] |
| The TV takes one host connection at a time. While Tizen Studio's sdb server held one, a second TCP connection to 26101 was accepted, then reset within 10 ms, before any packet. Once the server disconnected, the same connection got the TV's banner. | H | |
| A TV answers only the host PC named in its Developer mode settings. | D | [sam-device] |

## 2. Packets

| Fact | Tag | Source |
|---|---|---|
| A message is a 24-byte header of six little-endian 32-bit words (command, arg0, arg1, data length, data check, magic = command ^ 0xffffffff) and its payload. An invalid header must close the connection. | D | [adb-protocol] |
| sdb and the TV use adb's commands: `CNXN` 0x4e584e43, `OPEN` 0x4e45504f, `OKAY` 0x59414b4f, `WRTE` 0x45545257, `CLSE` 0x45534c43. | D, H | [adb-protocol] |
| The data check is the sum of the payload bytes, modulo 2^32, not a CRC: `host::\0` carries 0x232. Both sides fill it in. | H | |
| The host opens with `CNXN(0x00100000, 0x40000, "host::\0")`. The TV answers `CNXN(0x02000000, 0x3ff9c, "device::GQ75LS03DAUXZG::0\0")`: 0x3ff9c (262,044) is the largest payload it takes. | H | |
| The model in the banner is what `sdb devices` prints as the device's name. `capability:` says `device_name:Tizen`. | H | |
| No `AUTH` packet was ever sent by the TV. | H | |
| A `WRITE` payload "MUST be <= maxdata"; sdb's writes to the TV were up to exactly 262,044 bytes. | D, H | [adb-protocol] |

## 3. Streams

| Fact | Tag | Source |
|---|---|---|
| `OPEN(local-id, 0, "destination")` gets `READY` (`OKAY`) or `CLOSE`. Ids are relative to the sender. | D | [adb-protocol] |
| After a `WRITE`, no other `WRITE` may go out on that stream until the next `READY`. The TV acknowledged every `WRTE` with `OKAY`. | D, H | [adb-protocol] |
| A `CLOSE` "MAY" carry local-id 0. The TV sent every `CLSE` with arg0 0, and so did sdb. | D, H | [adb-protocol] |
| The TV refuses a service it does not run with `CLSE` and no `OKAY`: sdb's `shell:/usr/bin/pkginfo --pkg <id> \| grep ...` got that. `shell:ls /` got `OKAY`, then `CLSE` and no output. | H | |
| The TV writes a service's answer, then its `CLSE`, without waiting for the host's `OKAY`. | H | |

## 4. Services the TV answered

| Service | Answer | Tag |
|---|---|---|
| `capability:` | a uint16 little-endian byte count (0x02da), then that many bytes of `key:value\n` lines. Among them `cpu_arch:armv7`, `platform_version:9.0`, `profile_name:tv`, `sdk_toolpath:/home/owner/share/tmp/sdk_tools`, `vendor_name:Samsung`, `secure_protocol:enabled`, `appcmd_support:disabled`, `sdbd_version:2.2.31` | H |
| `shell:0 getduid` | the TV's DUID, then `\r\n` | H |
| `shell:0 getvmname` | `0\r\n` | H |
| `shell:0 execute <appid>` | `... successfully launched pid = <pid> with debug 0\r\n`. For an app that is not installed: `The app with ID: <appid> is not available for the user 5001` | H |
| `shell:0 vd_appinstall <pkgid> <path>` | lines as the install runs: `install <pkgid>`, `package_path <path>`, `app_id[<pkgid>] install start`, `app_id[<pkgid>] installing[<n>]` from 9 to 100, `app_id[<pkgid>] install completed`, `spend time for wascmd is [<ms>]ms` | H |
| the same, for a package whose file no longer matches its signature | `app_id[<pkgid>] install failed[118, -12], reason: Check certificate error : :Invalid signature. Signed with wrong key, changed signature file or changed package file.:<-4>` | H |
| `sync:` | adb's sync service, section 5 | D, H |
| `sysinfo:` | 320 bytes of NUL-padded fields, among them `2.2.0` and the model | H |

Samsung documents that "sdb logging feature is not supported on Samsung TVs"
([sam-testfaq] Q22); `dlog`, `sdb shell` and `sdb pull` are blocked on the same
TV ([tizen-tv-platform.md](tizen-tv-platform.md) section 5).

## 5. Pushing a file

| Fact | Tag | Source |
|---|---|---|
| `sync:` takes eight-byte requests: four ASCII letters, then a little-endian uint32. | D | [adb-sync] |
| `SEND` names `<path>,<mode>`, the mode in decimal; then `DATA` chunks of at most 64 KiB each; then `DONE` with the file's modification time. The server answers `DONE` with `OKAY`, or `FAIL` and a message. | D | [adb-sync] |
| sdb pushed a package as `SEND` `/home/owner/share/tmp/sdk_tools/tmp/<file>,493` (493 is `0755`), `DATA` chunks of 65,536 bytes packed back to back into `WRTE`s of up to 262,044 bytes, `DONE`, then `QUIT` once the TV said `OKAY`. | H | |
| A push into a directory that did not exist yet under `sdk_tools/tmp/` succeeded. A push to `/opt/` got `FAIL` `You cannot push files to this path.` | H | |
| A 7.5 MB package took 1.3 s to push over Wi-Fi. | H | |

## 6. What Tizen Studio sends to install and launch

`tizen install -s <serial> -n <file>.tpk -- <dir>` sent, in this order (H):

1. `capability:`.
2. `shell:0 getvmname`.
3. `shell:0 list_showcontrol "<dir>"` for `/`, `/home`, and each directory down
   to `<sdk_toolpath>/tmp`, twice over. Each got `OKAY`, then `CLSE` and no output.
4. `sync:`, pushing the file to `<sdk_toolpath>/tmp/<file>`.
5. `capability:` again, then `shell:0 vd_appinstall <pkgid> <sdk_toolpath>/tmp/<file>`.

Nothing removed the pushed file afterwards (H).

`tizen run -s <serial> -p <appid>` sent `capability:`, `shell:0 getvmname`, three
`shell:/usr/bin/pkginfo` queries the TV refused, `sysinfo:`, and
`shell:0 execute <appid>` (H).

## 7. What this package's client does

- One connection per command, closed when the command ends, so the TV's only
  connection is free between commands (section 1).
- The handshake, `capability:`, the push to `<sdk_toolpath>/tmp/<file>` and
  `0 vd_appinstall`, and `0 execute` as Tizen Studio sends them. It skips
  `getvmname`, `list_showcontrol`, `pkginfo` and `sysinfo:`: installs and
  launches on the TV work without them (H, 2026-11-03).
- Package and app ids must match `[A-Za-z0-9_.]+` and file names are cut to
  `[A-Za-z0-9._-]` before they reach a command line: how the TV parses one is
  not documented (I).
- An install succeeds when the TV prints `install completed`; a launch when it
  prints `successfully launched` (section 4).

## Unknowns

- What `0 getvmname` and `0 list_showcontrol` are for.
- Whether TVs from other model years answer `0 vd_appinstall` and `0 execute`
  as this Tizen 9.0 set does.
- Whether a TV ever asks for adb's RSA authentication.

## Sources

[td-sdb]: https://github.com/Samsung/tizen-docs/blob/master/docs/sdk-tools/baseline-sdk/common-tools/smart-development-bridge.md
[sam-device]: https://developer.samsung.com/smarttv/develop/getting-started/using-sdk/tv-device.html
[sam-testfaq]: https://developer.samsung.com/smarttv/develop/faq/application-testing.html
[adb-protocol]: https://android.googlesource.com/platform/system/core/+/refs/heads/android10-release/adb/protocol.txt
[adb-sync]: https://android.googlesource.com/platform/system/core/+/refs/heads/android10-release/adb/SYNC.TXT
