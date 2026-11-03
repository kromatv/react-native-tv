# Tizen TV platform reference

What Samsung and Tizen document about the platform react-native-tizen runs on,
checked on 2026-11-02. Every row names its source; the links are at the end.

| Tag | Meaning |
|---|---|
| **D** | Documented: an official page, a Tizen or TizenFX source file, or a file shipped in Tizen Studio |
| **I** | Inferred from documented facts; not stated anywhere |
| **H** | Verified on hardware: GQ75LS03DAUXZG, 2024 Frame, Tizen 9.0, armv7 userland, Developer mode, Public-level Samsung certificate |
| **O** | Observed on the build machine (Tizen Studio 10.0 on an arm64 Mac), not on a TV |

`docs.tizen.org` now answers every page with a 301 to the root of
`samsungtizenos.com/docs`, so Tizen docs are cited from their source repository,
[Samsung/tizen-docs][td].

## 1. App model

### 1.1 What the store takes

| Fact | Tag | Source |
|---|---|---|
| Seller Office takes two package formats: `WGT, TPK`. | D | [so-pub] |
| flutter-tizen registers TV apps as "Tizen .NET Application (TPK package)". | D | [ft-publish] |
| flutter-tizen's C++ app template is "not compatible with TV devices"; C# is the default. | D | [ft-commands] |
| "Tizen .NET is supported since Tizen 4.0 for Smart TV." | D | [sam-gs] |
| .NET and the TizenFX TV framework exist only on Samsung Smart TVs, not hotel TVs. | D | [sam-netfaq] Q5 |
| A native-only (C) TPK installs and "launches" but never runs. | H | |
| A .NET 6 app runs and its `DllImport` loads a C++ `.so` from the package's `lib/`. | H | |

### 1.2 Native code inside a .NET TPK

| Fact | Tag | Source |
|---|---|---|
| `DllImport` into TizenFX's own native libraries is forbidden "for security reasons". | D | [sam-netfaq] Q3 |
| `DllImport` into "signed native ".so" libraries within your application package" is allowed. "You must be a member of a Seller Office partner group to build or sign native ".so" libraries." | D | [sam-netfaq] Q3 |
| The package must not contain any TizenFX TV framework library (Tizen.Net, Tizen.Net.TV, the open .NET libraries the framework carries). | D | [sam-netfaq] Q4 |
| flutter-tizen puts `libflutter_engine.so`, `libflutter_tizen.so` and AOT `libapp.so` in the TPK's `lib/`, ICU data in `res/`. | D | [ft-package] |
| flutter-tizen binds with `[DllImport("flutter_tizen.so")]`; the file name is a constant because "the embedder so name is statically defined in C# code". | D | [ft-interop], [ft-package] |
| flutter-tizen native plugins are searched in `lib`, `lib/<arch>`, `lib/<arch>/<apiVersion>`. | D | [ft-plugins] |
| The distributor signature (`signature1.xml`) lists `lib/libreact-native-tizen.so` beside every other file, so the `.so` is signed by the same profile as the package. | O | built TPK |
| Samsung publishes Flutter apps (a .NET TPK carrying native `.so` files) on Samsung Smart TV Apps. | D | [sam-flutter] |

### 1.3 Certificates and privilege levels

| Fact | Tag | Source |
|---|---|---|
| A profile is one author certificate and one or more distributor certificates. The distributor certificate sets the privilege level and lists the target DUIDs. | D | [sam-cert] |
| Updates must be signed with the same author certificate as the original. | D | [sam-cert], [td-certs] |
| The developer's distributor certificate only allows install on registered devices; on store upload "it is replaced by the official distributor certificate". | D | [td-certs] |
| Apps using partner privileges are published only by members of a Seller Office partner group; testing them needs a partner-level distributor certificate with the test DUID. | D | [sam-privfaq] Q2, Q3 |
| Partner Seller membership is needed "to launch an application outside the United States or use certain APIs". | D | [so-partner] |
| Tizen 9.0 .NET privilege list: 56 Public, 1 Partner (`tee.client`). Native list: 62 Public, 20 Partner. | D | SDK `platforms/tizen-9.0/common/privileges/privilege-dotnet.properties`, `tizen/privileges/native/privilege-native.properties` |
| Samsung TV privileges (`http://developer.samsung.com/privilege/*`: `network.public`, `productinfo`, `drmplay`, ...) appear only in the web (WRT) list; `drminfo`, `sso.partner`, `camera` are partner. No .NET list exists for them. | D | SDK `platforms/tizen-9.0/tv-samsung/privileges/privilege-wrt.properties` |
| Since TV Extension 7.0.1 the emulator installs only Samsung-certificate-signed apps. | D | [sam-emu-policy] |
| A Public-level Samsung profile bound to the TV's DUID is enough to install, run .NET, and load the package's `.so` in Developer mode. | H | |

## 2. Model year, platform, runtime

`Initial` firmware per model year, from Samsung's platform features table
([sam-gs], Table 2) and the model-group table ([sam-mg]). glibc, ICU and EFL
come from the Tizen Studio rootstraps (`platforms/tizen-<v>/tizen/rootstraps/`).

| Year | Tizen | .NET framework | TizenFX API | GCC | glibc | ICU | EFL |
|---|---|---|---|---|---|---|---|
| 2026 | 10.0 | .NET 8.0 / netstandard2.1 | 13 | 14.2.0 | 2.40 | 76.1 | 1.25.1 |
| 2025 | 9.0 | net6.0 / netstandard2.1 | 12 | 9.2.0 | 2.30 | 74.1 | 1.25.1 |
| 2024 | 8.0 | net6.0 / netstandard2.1 | 11 | 9.2.0 | ? | ? | ? |
| 2023 | 7.0 | v2.1 | 10 | 9.2.0 | ? | ? | ? |
| 2022 | 6.5 | v2.1 | 9 | 9.2.0 | ? | ? | ? |
| 2021 | 6.0 | v2.1 | 8 | 9.2.0 | ? | ? | ? |
| 2020 | 5.5 | v2.1 | 6 | ? | ? | ? | ? |
| 2019 | 5.0 | v2.0 | 5 | ? | ? | ? | ? |
| 2018 | 4.0 | v2.0 | 4 | ? | ? | ? | ? |
| 2015 to 2017 | 2.3 to 3.0 | Not supported | | | | | |

- GCC column: "GCC 9.2.0 was used until the 2025 model, but GCC 14.2.0 was
  adopted from the 2026 model" ([sam-gs]); flutter-tizen says Tizen 6.0 to 9.0
  use 9.2.0 ([ft-embedder98]). Tizen 10.0 release notes: gcc 9.2.0 to 14.2.0,
  glibc 2.30 to 2.40, llvm 17.0.6 to 19.1.4 ([td-rn10]). D
- `?` means undocumented here: only the 9.0 and 10.0 rootstraps are installed.
- **OS upgrades.** "Samsung Smart TV from the 2023 lineup onwards will support
  OS Upgrade", so the model year no longer fixes the Tizen version; the table
  has identical `OS Upgrade` rows, meaning an upgraded TV runs the newer
  version's .NET and TizenFX ([sam-gs]). D. Press reports 2024 sets receiving
  Tizen 9.0 ([tomsguide], [ked]; secondary). The 2024 test TV runs 9.0. H
- **.NET runtime on the TV:** .NET 6.0.9 on Tizen 9.0. H. TV Extension
  emulators: 9.0 is ".NET 6 / TizenFX TV API 11.0.0", 10.0 is ".NET 8 /
  TizenFX TV API 12.12.0" ([sam-relhist]). D. The same release history
  reportedly lists .NET 6 for the 7.0 emulator where [sam-gs] says v2.1 for
  2023: unresolved.
- **TizenFX API levels** ([tizenfx]): API8 `tizen80` = Tizen 6.0, API9
  `tizen90` = 6.5, API10 `tizen10.0` = 7.0, API11 `net6.0-tizen8.0` = 8.0,
  API12 `net6.0-tizen9.0` = 9.0, API13 `net8.0-tizen10.0` = 10.0, API14
  `net8.0-tizen10.1`, API15 `net8.0-tizen11.0`. D. In the `Tizen.NET` 12.0.0.18510
  nuspec, TFM `Tizen9.0` pulls `Tizen.NET.API9` 9.0.0.16760 and
  `net6.0-tizen9.0` pulls `Tizen.NET.API12`. D (local `~/.nuget/packages/tizen.net/`)
- **CPU.** "For security reasons, the details are confidential" ([sam-devfaq]
  Q16). D. Tizen 9.0 rootstraps exist for armv7 (`device.core`) and aarch64
  (`device64.core`); 10.0 adds riscv64. D (SDK). flutter-tizen builds `arm`,
  `arm64` and `x86` ([ft-buildinfo]). D. Which models run an aarch64 userland:
  unknown. The test TV runs armv7. H. At runtime,
  `http://tizen.org/feature/platform.core.cpu.arch` names it ([sam-cap]). D
- **UI plane:** "Application: 1920x1080 (All models)"; 4K and 8K appear only as
  video resolutions ([sam-gs]). D
- **Memory:** "UHD model 380 MB, FHD model 300 MB", counting PSS + SWAP + GPU +
  GEM memory ([sam-hw]). Flash targets 5 MB install, 20 MB data, "not mandatory
  certification requirements" ([sam-hw]). D
- flutter-tizen supports TV from Tizen 6.0 (2021): "Flutter apps are not
  compatible with 2020 or older models" ([ft-readme], [ft-publish]). D

## 3. tizen-manifest.xml for a TV .NET app

| Element / attribute | Documented value | Seen in practice | Source |
|---|---|---|---|
| `manifest@api-version` | "x.y.z" with z optional, e.g. `2.4` | Samsung TV .NET sample `10.0`; flutter-tizen `6.0` | [td-manifest], [cs-sample], [ft-manifest] |
| `manifest@package` | a-z A-Z 0-9 `.` `-` `_`, under 50 characters | | [td-manifest] |
| `manifest@version` | x.y.z, x and y 0 to 255, z 0 to 65535 | | [td-manifest] |
| `profile@name` | `common`, `tv` | Samsung TV .NET samples and the SDK's TV web template: `tv`; flutter-tizen: `common` | [td-manifest], [cs-sample], SDK `tv-samsung/samples/Template/Web/Basic` |
| `ui-application@type` | `capp`, `c++app`, `webapp` (`dotnet` listed for component apps) | `dotnet` in every .NET TV sample | [td-manifest], [cs-sample] |
| `ui-application@exec` | executable path | the main DLL, e.g. `FirstScreen.dll` | [td-manifest], [cs-sample] |
| `ui-application@launch_mode` | `single` (default), `group`, `caller`; manifest `single` overrides a caller's request | `single` | [td-manifest], [td-appctl] |
| `ui-application@multiple` | `true`/`false`, "read-only" | `false` | [td-manifest] |
| `nodisplay`, `taskmanage` | `true`/`false`: app tray, task manager | `false`, `true` | [td-manifest] |
| `metadata` | key/value | `http://samsung.com/tv/metadata/mountinstall` = `true` (Samsung TV .NET samples, meaning undocumented); flutter-tizen: `http://tizen.org/metadata/prefer_dotnet_aot`, `prefer_nuget_cache` | [cs-sample], [ft-manifest] |
| `feature` | | `http://tizen.org/feature/screen.size.normal.1080.1920` (Samsung samples) | [cs-sample] |
| `background-category` | `media`, `download`, `background-network`, `location`, `sensor`, `iot-communication` | TV behaviour undocumented | [td-uiapp] |

Web-only TV metadata in the SDK's template: `http://samsung.com/tv/metadata/prelaunch.support`
and `http://tizen.org/metadata/app_ui_type/base_screen_resolution`. Whether a
.NET app honours them: unknown.

Privileges relevant here, all **Public** in the Tizen 9.0 .NET and native lists (SDK files above):

| Privilege | Use |
|---|---|
| `http://tizen.org/privilege/internet` | HTTP, WebSocket, the dev log stream |
| `http://tizen.org/privilege/network.get` | network state (NetInfo) |
| `http://tizen.org/privilege/mediastorage`, `externalstorage` | media folders, USB storage |
| `http://tizen.org/privilege/appmanager.launch` | launching other apps |
| `http://tizen.org/privilege/display` | display settings such as brightness; not needed to draw |
| `http://tizen.org/privilege/window.priority.set` | native list only; flutter-tizen requires it for its top-level window option ([ft-app]) |

### Lifecycle and keys

| Event | .NET (`CoreUIApplication`) | Native (`appfw/app.h`) | Documented trigger |
|---|---|---|---|
| Create | `OnCreate` | `app_create_cb` | before the main loop; create the window here. Returning false skips the loop |
| Launch request | `OnAppControlReceived` | `app_control_cb` | right after the main loop starts; again for each relaunch while running or paused; empty for a launcher start |
| Pause | `OnPause` | `app_pause_cb` | "completely obscured by another application"; a background app is suspended with limited CPU unless it declares a background category |
| Resume | `OnResume` | `app_resume_cb` | visible again |
| Terminate | `OnTerminate` | `app_terminate_cb` | `Exit()`, or "the low memory killer is killing your application" |
| Low memory | `OnLowMemory` | `APP_EVENT_LOW_MEMORY` (normal, soft, hard warning) | system memory pressure |

Sources: [td-lifecycle], [td-appctl], [td-uiapp], SDK
`rootstraps/tizen-9.0-device.core/usr/include/appfw/app.h` and `app_common.h`. D.
Samsung: "You must call the base class method within the override. Otherwise,
events are not emitted." ([sam-netbasics]). D. A launch URI arrives as
`ReceivedAppControl.Uri`, extras as `ExtraData` ([td-appctl]). D. Samsung's
deep-link or Smart Hub Preview contract for .NET apps: undocumented.

Remote keys for .NET apps, delivered through ElmSharp `EcoreEvent` KeyDown/KeyUp
([sam-netkeys]). D

| Key | Key name | Key | Key name |
|---|---|---|---|
| RETURN | `XF86Back` | EXIT, or RETURN long press (Smart Remote) | `XF86Exit` |
| ENTER | `Return` | arrows | `Up` `Down` `Left` `Right` |
| play/pause (Smart Remote) | `XF86PlayBack` | play, pause, stop | `XF86AudioPlayv` (sic), `XF86AudioPause`, `XF86AudioStop` |
| rewind, forward | `XF86AudioRewind`, `XF86AudioNext` | colour keys | `XF86Red` `XF86Green` `XF86Yellow` `XF86Blue` |
| CH+ / CH- | `XF86RaiseChannel` / `XF86LowerChannel` | TOOLS, INFO | `XF86SimpleMenu`, `XF86Info` |

What the TV does (documented for web apps; [sam-common], [sam-term], [sam-mt]):

- Return or Exit: "the application is terminated" (CO-GE-07). Exit on an app
  page "closes the application and shows the TV channel screen" (CO-US-04).
- Smart Hub (Home): the app goes to the background and Smart Hub shows; on TVs
  without multitasking the app terminates (CO-GE-07). "Models which have low
  memory specifications might not support multitasking."
- Long-pressing Return/Exit force-terminates: "Do not register the
  "Return/Exit" key long-press."
- Applications installed through Tizen Studio "are uninstalled when the TV is
  switched off or disconnected from the Tizen Studio" ([sam-testfaq] Q7).
- Whether a .NET app receives `XF86Exit` before the system terminates it:
  unknown.

## 4. Certification requirements

The checklists are written for web apps; none says it excludes .NET (I).

| Rule | Value | Test | Source |
|---|---|---|---|
| Return key policy | mandatory | | [sam-mand] |
| Return on a detail page | previous page | CO-US-05 | [sam-term], [sam-common] |
| Return on the home page | guide: an exit confirmation popup, exit only on "Yes". CO-US-05 expects the app to close and show Smart Hub with no popup mentioned | CO-US-05 | [sam-term], [sam-common] |
| Exit key | closes the app, TV channel plays | CO-US-04 | [sam-common] |
| Launch time | "at most 10 seconds ... from clicking the application icon to displaying its first page. A 5-second launch time is ideal." | | [sam-mand] |
| Responsiveness | responds to keys within 30 s | CO-GE-01 | [sam-common] |
| Loading indicator | shown while loading or buffering | CO-CN-03 | [sam-common] |
| Multitasking | "The application must implement multitasking" (TV) | CO-MT-01 to 04 | [sam-mand], [sam-common] |
| After background during playback | resume in the same state or show the previous page | CO-MT-01 | [sam-common] |
| Network loss | a notification popup; no freeze; Return still works | CO-CN-01, 02 | [sam-mand], [sam-common] |
| Stability | "without errors, crashing, freezing, or unintended functionality" | CO-UI-07, CO-SE-01 | [sam-mand], [sam-common] |
| Resolution | design for 1920x1080; "must have a 16:9 aspect ratio" to be published | | [sam-res] |
| Memory | 380 MB UHD, 300 MB FHD (PSS+SWAP+GPU+GEM); whether it is a certification gate is not stated | | [sam-hw] |
| Toolchain | "it must include a compatible library"; GCC 9.2.0 through 2025 products, GCC 14.2.0 from 2026; "compatibility is not guaranteed otherwise" | | [sam-flutter], [sam-gs] |
| Native `.so` | partner group membership to build or sign | | [sam-netfaq] Q3 |
| Distribution outside the US | Partner Seller membership | | [so-partner], [ft-publish] |
| Review time | "may take more than 4 weeks" (flutter-tizen) | | [ft-publish] |
| Crash logs | "TV applications do not support generating crash logs"; Samsung suggests a watchdog | | [sam-testfaq] Q16 |

"Compatible library" is not defined anywhere read: no ABI, symbol-version or
`libstdc++` rule is published.

## 5. Developer workflow

| Fact | Tag | Source |
|---|---|---|
| Developer mode: Apps panel, type `12345`, switch Developer mode on, enter the host PC's IP, reboot; "Develop Mode" then shows on the Apps panel. | D | [sam-device] |
| With Samsung Instant On, power-cycle at the reboot step. | D | [sam-testfaq] Q3 |
| How long Developer mode lasts: undocumented. | | |
| `sdb connect <TV IP>`. "Port 26101 is an internal device port. It cannot be opened separately." | D | [sam-testfaq] Q5, [sam-device] |
| No root through sdb. "sdb logging feature is not supported on Samsung TVs." | D | [sam-testfaq] Q21, Q22 |
| On the retail test TV, `dlog`, `sdb shell` and `sdb pull` are blocked. | H | |
| Which sdb services a retail TV answers: undocumented by Samsung. Seen on this TV: `capability:`, `sync:` pushes under `/home/owner/share/tmp/`, and `shell:0 <command>` forms such as `0 vd_appinstall` and `0 execute`; see [sdb-protocol.md](sdb-protocol.md). | H | |
| The TV takes one sdb host connection at a time; a second one is reset before its banner. | H | [sdb-protocol.md](sdb-protocol.md) |
| Tizen Studio error codes: 1009 uninstall fail, 1010 install fail (often certificates), 1013 run fail. | D | [sam-device] |
| Samsung's CLI page says "Only ".wgt" files can be installed on a TV" and that `tizen run -p` takes the application id. The first is dated: `.tpk` installs. | D, H | [sam-cli] |
| `tizen install` and `tizen run` against an unknown serial print "There is no <serial> target." and exit 1. | O | |
| `tizen package` exits 0 when signing did not happen; success is read from its output. `rn-tizen` no longer runs it: see [tizen-package-signing.md](tizen-package-signing.md). | O | |
| A successful `tizen run` means the launch request was sent, not that the app runs (the native-only TPK "launched"). | H | |
| With .NET SDK 10.0.107 and `Tizen.NET.Sdk/1.2.1`, a `net6.0-tizen9.0` project fails with `NETSDK1139: The target platform identifier tizen was not recognized` until the Tizen .NET workload is installed; `tizen90` builds with the SDK alone. | O | 2026-11-02, generated shell in a temp directory |
| The generated shell (`tizen90`, Tizen.NET 12.0.0.18510, `<profile name="tv" />`, `api-version="9.0"`) builds to `bin/Release/tizen90/<package>-<version>.tpk`, signed with Tizen.NET's default certificate, and the manifest inside keeps both values. | O | same |
| CLI exit codes: undocumented. | | [sam-cli] |
| Emulator: QEMU, needs Intel VT-x on macOS and Windows; Parallels and other VMs unsupported. | D | [sam-emu], [sam-prereq] |
| Emulator lacks UHD resolution, long-press, Bluetooth, camera; TizenFX TV `Accessory`, `Multimedia`, `Service.Billing`, `Service.Sso`. | D | [sam-emu-limits], [sam-relhist] |
| The installed TV image is `tv-samsung-10.0-x86_64` (base 10.0.6). On the arm64 Mac it boots in about 80 s of kernel time under emulation, then `launchpad-process-pool` segfaults. | O | `~/tizen-studio-data/emulator/vms/T-samsung-10.0-x86_64` |
| flutter-tizen targets TV emulators with `--target-arch x86`. | D | [ft-commands] |
| Tizen Studio ships `arm-linux-gnueabi` and `aarch64-linux-gnu` GCC 9.2 and 14.2 toolchains, `i586` 9.2, `riscv64` 14.2; sdb 4.2.36. | O | `~/tizen-studio/tools/` |

## 6. Implications for react-native-tizen

Rules:

1. **Ship a .NET TPK, native code in `lib/`.** A native-only TPK never runs (H),
   flutter-tizen calls C++ apps incompatible with TV ([ft-commands]), and
   flutter-tizen proves the `lib/*.so` plus `DllImport` layout ([ft-package]).
2. **Store release needs Partner Seller status.** Samsung requires partner
   group membership to "build or sign native ".so" libraries" ([sam-netfaq]),
   and outside the US to publish at all ([so-partner]). A Public certificate is a
   development-only path (H). Toolchain work cannot remove this gate.
3. **Keep the `.so` self-contained.** Static `libstdc++`/`libgcc`, exports
   limited to `rn_tizen_*` (`runtime/host/exports.map`), ICU compiled in with
   its symbols hidden instead of system ICU (74.1 on 9.0, 76.1 on 10.0 change
   the soname; see [intl.md](intl.md)). Link only C APIs from the rootstrap
   (EFL, appfw, dlog).
4. **Build against the oldest glibc you target.** 9.0 ships glibc 2.30, 10.0
   ships 2.40 (SDK, [td-rn10]); a 2.30 build should load on 2.40 (I, not
   verified on a Tizen 10 TV).
5. **One arch per TPK, chosen from the TV.** armv7 is required for at least
   some 2024 sets (H); aarch64 sysroots exist and the mapping to models is
   secret ([sam-devfaq]). flutter-tizen also builds one arch per package (I).
6. **Target 1920x1080 and 380 MB.** Lay out at 1080p on every panel ([sam-gs],
   [sam-res]); the budget counts Hermes heap, Evas buffers and GPU memory
   together ([sam-hw]).
7. **First frame inside 10 s, with a loading indicator** ([sam-mand], [sam-common]).
8. **Back key.** Map `XF86Back` to React Native's back handling. On the root
   screen, show an exit confirmation and call `Exit()` on "Yes" ([sam-term]).
   Never act on a long press of Return ([sam-term]). Treat `XF86Exit` as
   system-owned.
9. **Pause means stop.** Stop playback, timers and network jobs in `OnPause`;
   persist state there, since a low-memory kill ends in `OnTerminate` at best and
   background apps get limited CPU ([td-lifecycle]). Check the network and
   sessions in `OnResume` ([sam-mt]).
10. **Handle every relaunch.** `OnAppControlReceived` runs on first launch and
    on each later launch request; forward `Uri` every time ([td-appctl]).
11. **Logs travel over the app's own socket.** dlog is unavailable on retail
    sets ([sam-testfaq] Q22, H) and crash logs do not exist ([sam-testfaq] Q16):
    the TCP log stream and `StartupFailure` are the only diagnostics.
12. **Reinstall every session.** Dev installs vanish at power-off ([sam-testfaq] Q7).
13. **Trust output and the app, not exit codes.** Read `tizen` output (O) and
    confirm a launch by the app's first log line (H).
14. **The emulator is no test target on Apple Silicon.** It is x86_64-only (O)
    and needs VT-x ([sam-prereq]); the repo's own simulator stays the fallback.

Where the current approach disagrees with the documentation:

| Current | Documentation | Action |
|---|---|---|
| `shell/tizen-manifest.xml` said `<profile name="tv-samsung" />` | documented values are `common` and `tv`; Samsung's TV .NET samples and TV template use `tv` ([td-manifest], [cs-sample]) | done: `<profile name="tv" />`; `tv-samsung` is Tizen Studio's platform id, not a manifest value |
| `api-version="9"` | format "x.y.z", z optional; samples write `10.0` ([td-manifest], [cs-sample]) | done: `9.0`, the platform version |
| `platform-profiles.ts` paired Tizen `9.0` with TFM `tizen90` as if one named the other | `tizen90` is API9 = **Tizen 6.5**; Tizen 9.0 is API12, TFM `net6.0-tizen9.0` ([tizenfx], nuspec) | done: each platform names a TizenFX API level and its TFM comes from the level table (`TIZENFX_API_LEVELS`). Tizen 9.0 keeps API9 (`tizen90`): the shell calls nothing newer, it is the one verified on the TV (H), and API12 needs the Tizen .NET workload (O, below). For Tizen 10 the TFM is `net8.0-tizen10.0`, while `tizen10.0` means Tizen 7.0 |
| README: `rn-tizen package` builds a release package "for the store" | native `.so` in a store package requires partner group membership ([sam-netfaq]) | say so in the README and `doctor`; a Public profile only covers Developer mode |
| clang + lld with static GCC 14 libstdc++ for Tizen 9.0 (2025) TVs | Samsung: 2025 and earlier products used GCC 9.2.0 and apps must "include a compatible library" ([sam-flutter], [sam-gs]) | works on hardware (H); store acceptance is unknown. Keep the compat shim and hidden symbols, and ask the Seller Office Content Manager before a release |

Unknowns worth a hardware or Samsung answer:

- Store review of a .NET TPK whose `.so` is built by clang with a static GCC 14 `libstdc++`.
- Which model years and panels run an aarch64 userland.
- glibc, ICU and .NET runtime on Tizen 6.0 to 8.0 sets; the 2023 framework (v2.1 or .NET 6).
- Whether a `tizen90` (API9) shell runs on Tizen 10.0's .NET 8.
- Whether .NET apps see `XF86Exit`; Smart Hub deep links and prelaunch for .NET apps.
- Developer mode lifetime, and the meaning of `mountinstall`.

## 7. Graphics resolution on UHD panels

The size of the app's own window on a 4K set. The video plane is separate and
decodes at 4K on its own ([tizen-media-playback.md](tizen-media-playback.md),
section 3). Checked on 2026-11-03. **R** marks a third party's report from
their own set.

### What is documented

| Fact | Tag | Source |
|---|---|---|
| "Application: 1920x1080 (All models)"; video 3840x2160 on 4K UHD models, 7680x4320 on 8K | D | [sam-gs] Table 1 |
| Standard app resolution: UHD TV 1920x1080, FHD TV 1280x720. "When the application screen resolution is larger than the standard resolution, it is automatically resized to fit the screen." "Always design your application for a 1920x1080 px screen" | D | [sam-res] |
| The one documented per-app resolution metadata is `http://tizen.org/metadata/app_ui_type/base_screen_resolution` = `extensive`, for 21:9 panels of the `23TV_PREMIUM2` group, where a web app's `window.innerWidth` becomes 2560 at a height of 1080 | D | [sam-21x9] |
| `webapis.productinfo.isUdPanelSupported()`: "true : 4K or higher resolution supported" (web only); it describes the panel, not the app's surface | D | [sam-4kvideo] |
| The TV emulator has no UHD resolution | D | [sam-emu-limits] |
| The 380 MB UHD budget counts GPU memory, "used by the GPU to render the application's UI, textures, and display output", and GEM memory, "graphics buffer memory used for display output" | D | [sam-hw] |
| Native window APIs on Tizen 9: `ecore_wl2_display_screen_size_get` (`ecore-wl2-1/Ecore_Wayland2.h:264`), Tizen-only `ecore_wl2_display_all_screens_size_get` (`:276`), `elm_win_screen_size_get` (`elementary-1/elm_win_legacy.h:885`), `ecore_evas_screen_geometry_get` (`ecore-evas-1/Ecore_Evas.h:3138`), window auxiliary hints `elm_win_aux_hint_add` since Tizen 5.5 (`elm_win_legacy.h:1528`). No header names a resolution request | D | SDK `rootstraps/tizen-9.0-device.core/usr/include` |
| flutter-tizen sizes its window to `ecore_wl2_display_screen_size_get` and, on TV, computes its pixel ratio from a fixed 72 DPI and a profile factor of 2.0 (1.6); nothing in it asks for a 4K surface | D | [ft-wl2] lines 196-206, [ft-sysutils] lines 13-33 |
| Web apps stay at 1080p with a 3840 viewport and `screen-resolution="3840x2160"`; the project owner blames "the limitations of Tizen's internal web engine" | R | [tt-39] |
| A web app on a 2024 4K set running Tizen 9 "rendered on an upscaled 1920 surface and looked soft" | R | [ht-10] |
| Samsung signage players are "set to a 1920x1080 resolution as this is the highest resolution Samsung (Tizen) ... will allow" (Xibo's maintainer, not Samsung) | R | [xibo-4k] |

### What the TV does

| Fact | Tag | Where |
|---|---|---|
| Which models configure a 3840x2160 output | U | |
| On the GQ75LS03DAUXZG (The Frame, 2024, Tizen 9.0), with no metadata, with `base_screen_resolution` `4k_only` and with `max`, every source reports 1920x1080: `system_info` `screen.width`/`height`, `elm_win_screen_size_get`, `ecore_evas_screen_geometry_get`, `ecore_wl2_display_screen_size_get` and `ecore_wl2_display_all_screens_size_get` | H | the host's `Screen:` log line |
| On that TV, a window resized to 3840x2160 right after creation is 1920x1080 again two seconds later, with a 1920x1080 canvas: the window manager does not grant a larger window | H | same |

So the test TV, a 4K set, gives an app a 1920x1080 window whatever its
metadata asks; whether any model grants more, and what each value grants, is
undocumented. U

### What it costs

Arithmetic on RGBA8888 buffers, against the 380 MB UHD budget ([sam-hw]). I

| | 1920x1080 | 3840x2160 |
|---|---|---|
| One window buffer | 8.3 MB | 33.2 MB |
| Three buffers (the EGL swap chain's real count is U) | 24.9 MB, 6.5% of the budget | 99.5 MB, 26% |
| Pixels shaded by a full redraw | 2.07 M | 8.29 M |
| One full-screen decoded image | 8.3 MB | 33.2 MB |

### What react-native-tizen should do

1. **Keep 1920x1080 logical points.** The host sizes the window from
   `system_info` `screen.width` and `screen.height`, 1920x1080 when absent
   (`runtime/host/src/modules/DeviceProfile.cpp:37-38`,
   `runtime/host/src/ReactHostWindow.cpp:10-13`), and draws at
   `min(width / 1920, height / 1080)` (`runtime/host/src/mounting/LogicalScreen.cpp`).
   A 3840x2160 window already yields a point scale of 2 with no other change
   ([decisions.md](../decisions.md), "One logical 1920x1080 screen").
2. **Make 4K an opt-in.** Samsung recommends 1080p and a 4K swap chain takes
   a quarter of the memory budget. An app setting `expo.tizen.resolution` to
   `"4k"` gets `<metadata key="http://tizen.org/metadata/app_ui_type/base_screen_resolution" value="4k_only" />`
   on its `ui-application`; the default writes nothing. `4k_only` rather than
   `max`, whose name promises the largest output: on an 8K set three
   7680x4320 buffers alone (398 MB) would exceed the budget (I).
3. **Size the window from the compositor, not `system_info`.** `screen.width`
   is a static platform feature (I: it stays 1920 whatever the window manager
   grants); read `elm_win_screen_size_get` after `elm_win_add`, falling back to
   `system_info`, and log both.
4. **What changes at a scale of 2.** Text is rasterized at twice the pixel
   size, so it is sharp instead of upscaled, and the glyph cache holds four
   times the pixels. Images decode near their drawn size (`load_size_set`), so
   four times the pixels and texture memory; the Evas image cache
   (`evas_image_cache_set`) needs sizing to match. Nine-patches and CPU
   shadows are generated at twice the radius and cached as before. A full
   redraw shades four times the pixels, and a snapshot blur costs four times
   as much ([efl-rendering.md](efl-rendering.md), "Backdrop blur"). The video
   ROI is passed in window pixels ([tizen-media-playback.md](tizen-media-playback.md),
   section 3), so it doubles with the scale.

### What react-native-tizen does

Built as points 1 to 3 say ([`../tv-features.md`](../tv-features.md#4k-ui)):
`expo.tizen.resolution: "4k"` writes the `4k_only` metadata and `resolution`
into the runtime config; `fitWindowToScreen` (`runtime/host/src/tv/ScreenSize.cpp`)
reads `elm_win_screen_size_get` and `ecore_evas_screen_geometry_get` right
after the window is created, resizes it to the window manager's screen when
the app asked for 4K and that screen is at least 3840x2160, and logs every
source, the window and the canvas, then the window and canvas again two
seconds later. `DeviceInfo`, `ExpoTizen.getScreenSize` and the surface then
take the new size, so the point scale is 2. Without the opt-in nothing is
resized and only the log line is new. In the simulator every source reports
1920x1080, and so does the 2024 Frame (rows above), where the opt-in therefore
changes nothing: no 4K surface, so no frame cost to measure there. A 4K set
whose window manager reports 3840x2160 is still to be found.

### Verify on a TV

On the 2024 Samsung The Frame (LS03D), Tizen 9.0:

1. With no metadata, log `system_info` `screen.width` and `screen.height`,
   `elm_win_screen_size_get`, `ecore_wl2_display_screen_size_get`,
   `ecore_wl2_display_all_screens_size_get`, `ecore_evas_screen_geometry_get`,
   `ecore_wl2_output_dpi_get`, the window's geometry once shown, and
   `GL_MAX_TEXTURE_SIZE`.
2. Repeat with `base_screen_resolution` set to `max`, then to `4k_only`.
3. If a window reports 3840x2160: capture a screenshot through `rn-tizen` and
   check its pixel size, scroll a rail and log frame times, and read the
   memory the app uses.
4. With a 4K window, play a video in a view and check that the picture
   follows its rect.

## Sources

Local: Tizen Studio 10.0 at `~/tizen-studio` (privileges, rootstraps, templates,
tools), `~/tizen-studio-data/emulator`, `~/.nuget/packages/tizen.net/12.0.0.18510/tizen.net.nuspec`,
and this package's built `example/tizen/bin/Release/tizen90/*.tpk`.

[td]: https://github.com/Samsung/tizen-docs
[td-manifest]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/native/reference/manifest-xml.md
[td-lifecycle]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/applications/uiapplication/application-lifecycle.md
[td-uiapp]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/applications/uiapplication/ui-app.md
[td-appctl]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/exchange-data-between-apps/app-controls.md
[td-certs]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/concepts/signing-certificates/index.md
[td-rn10]: https://github.com/Samsung/tizen-docs/blob/master/docs/platform/release-notes/tizen-10-0.md
[tizenfx]: https://github.com/Samsung/TizenFX/blob/main/README.md
[cs-sample]: https://github.com/Samsung/Tizen-CSharp-Samples/tree/master/TV/UI/FirstScreen/FirstScreen
[sam-gs]: https://developer.samsung.com/smarttv/develop/specifications/general-specifications.html
[sam-mg]: https://developer.samsung.com/smarttv/develop/specifications/tv-model-groups.html
[sam-cap]: https://developer.samsung.com/smarttv/develop/api-references/tizen-web-device-api-references/systeminfo-api/getting-device-capabilities-using-systeminfo-api.html
[sam-devfaq]: https://developer.samsung.com/smarttv/develop/faq/device-information-and-firmware.html
[sam-hw]: https://developer.samsung.com/smarttv/develop/ai-tv-platform-standards/hw-resource-management.html
[sam-netfaq]: https://developer.samsung.com/smarttv/develop/faq/tizen-net-tv.html
[sam-netbasics]: https://developer.samsung.com/smarttv/develop/tizen-net-tv/guides/basics.html
[sam-netkeys]: https://developer.samsung.com/smarttv/develop/tizen-net-tv/guides/user-interaction.html
[sam-flutter]: https://developer.samsung.com/smarttv/develop/native/flutter.html
[sam-relhist]: https://developer.samsung.com/smarttv/develop/tools/tv-extension/release-history.html
[sam-mand]: https://developer.samsung.com/smarttv/develop/development-checklist/mandatory-features.html
[sam-common]: https://developer.samsung.com/smarttv/develop/development-checklist/common.html
[sam-term]: https://developer.samsung.com/smarttv/develop/guides/fundamentals/terminating-applications.html
[sam-mt]: https://developer.samsung.com/smarttv/develop/guides/fundamentals/multitasking.html
[sam-res]: https://developer.samsung.com/smarttv/develop/guides/fundamentals/managing-screen-resolution.html
[sam-device]: https://developer.samsung.com/smarttv/develop/getting-started/using-sdk/tv-device.html
[sam-cert]: https://developer.samsung.com/smarttv/develop/getting-started/setting-up-sdk/creating-certificates.html
[sam-privfaq]: https://developer.samsung.com/smarttv/develop/faq/privileges.html
[sam-testfaq]: https://developer.samsung.com/smarttv/develop/faq/application-testing.html
[sam-cli]: https://developer.samsung.com/smarttv/develop/getting-started/using-sdk/command-line-interface.html
[sam-emu]: https://developer.samsung.com/smarttv/develop/getting-started/using-sdk/tv-emulator.html
[sam-emu-limits]: https://developer.samsung.com/smarttv/develop/getting-started/using-sdk/tv-emulator/emulator-limitations.html
[sam-emu-policy]: https://developer.samsung.com/smarttv/develop/getting-started/using-sdk/tv-emulator/application-install-policy.html
[sam-prereq]: https://developer.samsung.com/smarttv/develop/tools/prerequisites.html
[so-pub]: https://developer.samsung.com/tv-seller-office/application-publication-process.html
[so-partner]: https://developer.samsung.com/tv-seller-office/guides/membership/becoming-partner.html
[ft-readme]: https://github.com/flutter-tizen/flutter-tizen/blob/master/README.md
[ft-publish]: https://github.com/flutter-tizen/flutter-tizen/blob/master/doc/publish-app.md
[ft-commands]: https://github.com/flutter-tizen/flutter-tizen/blob/master/doc/commands.md
[ft-manifest]: https://github.com/flutter-tizen/flutter-tizen/blob/master/templates/ui-app/csharp/tizen-manifest.xml.tmpl
[ft-package]: https://github.com/flutter-tizen/flutter-tizen/blob/master/lib/build_targets/package.dart
[ft-plugins]: https://github.com/flutter-tizen/flutter-tizen/blob/master/lib/build_targets/plugins.dart
[ft-buildinfo]: https://github.com/flutter-tizen/flutter-tizen/blob/master/lib/tizen_build_info.dart
[ft-interop]: https://github.com/flutter-tizen/flutter-tizen/blob/master/embedding/csharp/Tizen.Flutter.Embedding/Interop/Interop.cs
[ft-app]: https://github.com/flutter-tizen/flutter-tizen/blob/master/embedding/csharp/Tizen.Flutter.Embedding/FlutterApplication.cs
[ft-embedder98]: https://github.com/flutter-tizen/embedder/pull/98
[ft-wl2]: https://github.com/flutter-tizen/embedder/blob/44c61a1f069a11a166fbeef0cae17855e152ab40/flutter/shell/platform/tizen/tizen_window_ecore_wl2.cc
[ft-sysutils]: https://github.com/flutter-tizen/embedder/blob/44c61a1f069a11a166fbeef0cae17855e152ab40/flutter/shell/platform/tizen/system_utils.cc
[sam-21x9]: https://developer.samsung.com/smarttv/develop/guides/base_screen_resolution.html
[sam-4kvideo]: https://developer.samsung.com/smarttv/develop/guides/multimedia/4k-8k-uhd-video.html
[tt-39]: https://github.com/reisxd/TizenTube/issues/39
[ht-10]: https://github.com/stanislavnastasiu-dev/HyperTizen/pull/10
[xibo-4k]: https://community.xibo.org.uk/t/tizen-native-4k-resolution/27925
[tomsguide]: https://www.tomsguide.com/tvs/samsung-is-giving-these-older-tvs-a-big-tizen-os-upgrade-see-if-yours-is-on-the-list
[ked]: https://amp.kedglobal.com/newsView/ked202408230003
