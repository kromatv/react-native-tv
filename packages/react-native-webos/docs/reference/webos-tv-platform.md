# webOS TV platform reference

What LG, webOS OSE and projects that ship on LG TVs document about the platform
react-native-webos runs on: a native process packaged as an `.ipk`. Checked on
2026-11-11. No TV was used: nothing here is verified on hardware.

| Tag | Meaning |
|---|---|
| **D** | Documented by LG: webostv.developer.lge.com, LG Seller Lounge, a file in LG's CLI (`@webos-tools/cli` 3.2.5) or in LG's flutter-webos repositories |
| **D-ose** | Documented for webOS OSE: webosose.org or `github.com/webosose` source. OSE shares SAM (the app manager) and luna-service2 with TV, but the TV's SAM and jailer are closed, so applying a D-ose row to a TV is an inference |
| **D-3p** | Documented by a third party that ships on TVs or reports from one: webosbrew (docs, SDL-webOS, firmware symbol data), Kodi, a forum post, an issue. Not LG's word, not checked by us |
| **I** | Inferred; the row says from what |

Repositories are cited at a pinned commit (see Sources) as `[label] path Lx-y`.
`[sam]` is today's OSE SAM (rewritten in 2020), `[sam1]` its first public
release (2018), which still carries the two native lifecycle versions TV apps use.

## 1. App model and store policy

### 1.1 App types

| Fact | Tag | Source |
|---|---|---|
| LG's public site documents web apps (packaged or hosted) and JavaScript services, nothing else. | D | [lg-discover], [lg-webtypes] |
| `type` in `appinfo.json`: "Only `web` is allowed currently." | D | [lg-appinfo] |
| LG's CLI accepts `type` in `web`, `stub`, `native`, `native_builtin`, `native_appshell`, `qml`; its TV profile defaults `main` to `executable` for native, but ships web and JS service templates only. | D | [cli] `files/schema/ApplicationDescription.schema` L67-77, `files/conf-base/profile/config-tv.json`, `files/conf-base/template-conf/tv-sdk-templates.json` |
| OSE has three types: `web`, `qml`, `native`. A native app is a C/C++ binary that calls services through luna-service2 and "must register itself" with SAM by `registerApp`. | D-ose | [ose-appinfo], [ose-native] |
| SAM accepts `qml` only for apps in the system directories, so a downloaded QML app does not load. `native` with `privilegedJail` becomes `native_mvpd`. | D-ose | [sam1] `src/core/package/application_description.cpp` L114-125 |
| LG ships Flutter for webOS TV: a `type: "native"` IPK holding an ELF runner and the engine `.so` files. "Flutter for webOS supports webOS TV starting from webOS 26 Re:New"; TVs shipped with webOS 26 need the Re:New update. | D | [lg-flutter], [lg-flutter-news], [fw] `templates/app/meta/appinfo.json.tmpl` |
| LG's own system apps moved to Flutter: the Japanese EPG on 2024 TVs, more apps on 2025 TVs, reaching older models through webOS Re:New. | D | [lg-flutter-2024] |

### 1.2 Who may publish a native app

| Fact | Tag | Source |
|---|---|---|
| LG's developer site states no store policy for native apps. The approval pages speak of an IPK, a UX scenario and a self checklist, never of an app type. The Seller Lounge FAQ does not contain the word native. | D | [lg-approval], [lg-eco], [seller-faq] |
| Seller Lounge's upload form does take native packages: an uploaded IPK's File Type is "automatically configured", Web for a web app and otherwise the "Seller-registered File Type (Native/Native Game)". A file is uploaded for a chipset, and "If you do not have a separate standing contract with LGE, select web > All". So a native package is a contract matter, and one file is registered per chipset. | D | [seller-upload] |
| Sellers are individuals or companies; LG QA runs a pretest, a function test and a content test; every update is approved again. | D | [lg-eco], [lg-approval] |
| A developer who asked for the NDK in 2023 was told a partnership agreement was needed, then got this from Seller Lounge: "it seems that the NDK feature cannot be supported even when the deal proceeds". | D-3p | [forum-3365] |
| LG staff, November 2026: "This forum is only for webOS TV web apps." | D | [forum-28525] |
| Partnership is requested at Seller Lounge, 1:1 QA > Partnership > Business Partnership. The Partner Zone offers "partner exclusive APIs and resources". | D | [lg-contact], [lg-partner] |
| flutter-webos sends its users to the Partner Zone for support, and LG keeps a `plugins-beta` repository for "authorized partners only". | D | [fw] `doc/getting-started.md` L5, [fw-org] |
| Whether Seller Lounge takes a Flutter IPK without such a contract: not stated. A developer asked in January 2026; staff answered only the release date. | D | [forum-28079] |
| webOS TV 27 makes `requiredACG` in `appinfo.json` "a mandatory item in the Seller Lounge eligibility check" for every app. | D | [lg-acg] |
| Store apps install under `/media/cryptofs/apps/` and need valid DRM info to run; homebrew never goes there. | D-3p | [wb] `filesystem.md`, `vocabulary.md` |

### 1.3 Developer Mode

| Fact | Tag | Source |
|---|---|---|
| The Developer Mode app comes from LG Apps, needs an LG Developer account and reboots the TV when switched on. | D | [lg-devmode] |
| "Developer Mode enables for a limited time." EXTEND resets the remaining time while the TV is online; "If the session time runs out, you cannot extend". | D | [lg-devmode] |
| The session is 1000 hours. One account keeps one TV in Developer Mode. | D-3p | [wb-devmode]; [fw-8] shows "~986 h remaining" |
| It turns off after ten reboots without a network, or on the reboot after the session ran out. Then "the installed apps ... are uninstalled". | D | [lg-devmode] |
| Access is SSH on port 9922 as `prisoner`, with a key fetched while Key Server is on, unlocked by the 6 character passphrase on screen. | D | [lg-devmode] |
| The CLI installs with `/usr/bin/luna-send-pub` on `com.webos.appInstallService/dev/install`, after copying the IPK to `/media/developer/temp`; it launches with `com.webos.applicationManager/launch` and closes with `.../dev/closeByAppId`. | D | [cli] `files/conf/command-service.json`, `files/conf-base/profile/config-tv.json` |
| A native IPK installs and runs in Developer Mode on a retail TV: LG's Flutter guide requires exactly that (webOS 26 Re:New or later). | D | [lg-flutter], [fw] `doc/getting-started.md` L23 |
| Homebrew does the same on older TVs: SDL2 apps back to webOS 1, Kodi on "webOS 5.0 or newer". | D-3p | [wb] `native/ui.md`, [kodi] `docs/README.webOS.md` L73 |
| SAM has a jail type of its own for apps found in the developer path: `native_devmode`. | D-ose | [sam] `src/bus/client/NativeContainer.cpp` L162-164 |
| SAM refuses a developer-path app whose id starts with `com.palm.`, `com.webos.` or `com.lge.`, or whose `trustLevel` is `trusted`. | D-ose | [sam1] `application_description.cpp` L160-163, L188-191, L421-422 |
| `/media/developer` was 984 MB on an 8 GB 2026 model; a 176 MB debug build failed with "There is no available space to install App". | D-3p | [fw-8] |
| LG's simulator runs web apps only: "there is no official webOS simulator" for native ones. LG's NDK installer runs on x86-64 Linux only. | D | [fw-9], [fw-10] (answered in LG's repository by a maintainer) |
| LG's webOS TV 26 Simulator 1.5.0 for macOS is an Electron app. It refuses an `appinfo.json` without `id`, `title`, `type`, `main`, `icon` or `version`, and starts an app by loading its `main` in a Chromium window (`webContents.loadURL("file://<main>")`). It starts no program of the app's and opens no port a TV has (SSH, the key server, the remote control service). LG's page lists it under Limitations: "Native apps are not supported (only web apps are supported)." Opened on the example's TV package on 2026-11-11, it reported a successful launch, made a page of `file://.../react-native-webos` with no element and no script, and stayed on its home screen. | D | [lg-sim-intro]; [lg-sim] `Contents/Resources/app.asar`, `.vite/build/main.js`, read on 2026-11-11 |

### 1.4 A real TV without owning one

| Fact | Tag | Source |
|---|---|---|
| LG runs a **webOS Cloud Test Lab** for sellers, in Seller Lounge under Applications: "App functions can be checked on real devices in each country. Devices are installed in Korea and use the same SW version as the production environment". The IPK is uploaded first (File Upload), and the app needs an English title and description. | D | [seller-main], [seller-cloudlab] |
| A session is reserved: one IPK and one device, chosen by platform, country and SoC, in one-hour slots, three hours at most, five devices a day. "For Native files, only devices with the same chipset as the chipset you selected when uploading the file can be selected." | D | [seller-cloudlab] |
| The session is a video of the TV (1280x720 at 10 frames a second by default, up to 30 below 600p) with Launch App, Re-install and Reboot. The page names no shell and no log. Release ends it: "the app will be deleted, and the device will be initialized". Testing "device operations, not apps" can suspend the access for three months. | D | [seller-cloudlab] |
| Alpha Test publishes a test version to up to 100 TVs named by their wired MAC address, from webOS 3.0 on; "For the Native Type, it can be checked on the devices for each chipset". It is "only opened to sellers that have entered a separate commercial contract for the time being". | D | [seller-alpha] |
| TV Labs, a company, rents sessions on real LG TVs. Its `tvlabs connect` opens a tunnel that maps local port 9922 to the TV's 9922 (SSH), 9991 to 9991 (the key server), 3000 and 3001 to the remote control service, and 9222 to the web inspector; the TV must be in Developer Mode, signed in to the developer's own account. `ares-install -d tvlabs` and `ares-launch` then work on it, and a build can also be dropped on a live session. | D-3p | [tvlabs-webos], [tvlabs-sideload] |
| TV Labs lists a free trial on request ("No credit card required", two devices, developer tools). | D-3p | [tvlabs] |
| webOS OSE, the open source edition, takes native apps officially: its tutorial builds a Wayland and EGL program with the OSE NDK, packages it with `ares-package` and installs it on the emulator or a board. Since 2.20.0 LG publishes ready images for the Raspberry Pi 4 (64-bit) and for the emulator (x86-64) on GitHub. OSE shares the app manager and the bus with TV (see the tags above), but its userland is 64-bit and it has none of the TV's services. | D-ose | [ose-native], [ose-2.20] |

None of these has been tried with this package.

## 2. appinfo.json for a native app

One JSON object at the app root. The CLI requires `id`, `main`, `icon`, `type`
and `title`, checks the file against its schema, and fills `version` with
`1.0.0` when missing ([cli] `lib/package.js` L545-604, schema L312-318). D

| Field | Rule | Tag | Source |
|---|---|---|---|
| `id` | Lowercase letters, digits, minus, period; starts alphanumeric; two characters or more; must not start with `com.palm`, `com.webos`, `com.lge`, `com.palmdts`; fixed once published. The CLI tests `/^[a-z0-9.+-]*$/`. With a JS service: no minus and no `.` followed by a digit. | D | [lg-appinfo], [cli] `lib/package.js` L591 |
| `version` | Three integers `major.minor.revision`, each up to nine digits, no leading zero, all three required. A version already uploaded cannot be uploaded again. | D | [lg-appinfo], [cli] `lib/package.js` L602 |
| `type` | `native`. LG's page allows `web` only; LG's Flutter template and CLI schema use `native`. | D | [lg-appinfo], [fw] `appinfo.json.tmpl` |
| `main` | Path of the executable, relative to `appinfo.json`. The CLI reads its ELF header to set the package architecture. | D-ose, D | [ose-appinfo], [cli] `lib/package.js` L920-978 |
| `title` | Mandatory; 20 characters or fewer. | D | [lg-appinfo], [lg-resources] |
| `vendor` | Optional owner name. | D | [lg-appinfo] |
| `icon`, `largeIcon` | PNG, 80x80 and 130x130. Both "required for app submission"; the store shows a 400x400 image uploaded apart. | D | [lg-appinfo], [lg-resources] |
| `iconColor` | Tile background, default `white`. | D | [lg-appinfo] |
| `bgImage`, `bgColor` | 1920x1080 PNG and a color: webOS TV 1.x and 2.x only, ignored later. | D | [lg-appinfo] |
| `splashBackground` | PNG, 1920x1080, shown "while your app is being loaded". The resources page lists it as required and says it "should NOT be a black screen". | D | [lg-appinfo], [lg-resources] |
| `noSplashOnLaunch`, `spinnerOnLaunch` | Default `false` and `true`. In the CLI schema and SAM, not on LG's page. LG's Flutter template sets `true` and `false`; Kodi sets `spinnerOnLaunch: false`. | D, D-ose, D-3p | [cli] schema L213-222, [sam1] `application_description.cpp` L261-266, [fw] `appinfo.json.tmpl`, [kodi] `tools/webOS/packaging/appinfo.json.in` |
| `resolution` | `"1920x1080"` (default) or `"1280x720"`. See section 6. | D | [lg-appinfo] |
| `requiredMemory` | Megabytes. LG: the minimum to run. CLI schema: the peak while launching, so the memory manager can free it before an OOM. SAM asks the memory manager before every launch. Kodi declares 500. | D, D-ose, D-3p | [lg-appinfo], [cli] schema L172-175, [sam] `src/manager/PolicyManager.cpp` L50-51, [kodi] `appinfo.json.in` |
| `handlesRelaunch` | LG's page: a web app handles `webOSRelaunch` in the background, then calls `PalmSystem.activate()`. CLI schema: "For native app. If true, SAM relaunch app when user execute native app that already running, If false, just maximize existing window." LG's Flutter template and Kodi set `true`. | D, D-3p | [lg-appinfo], [cli] schema L130-134, [fw] `appinfo.json.tmpl`, [kodi] `appinfo.json.in` |
| | Neither OSE SAM reads it. | I | no match for `handlesRelaunch` in the C++ of [sam] or [sam1] |
| `nativeLifeCycleInterfaceVersion` | 1 (default) or 2. Absent from LG's page and the CLI schema; read by SAM. Section 3.3 has the difference. Kodi uses 2; LG's Flutter runner defaults to 1 while LG's system Flutter apps use 2. | D-ose, D-3p | [sam1] `application_description.cpp` L207-208, [kodi] `appinfo.json.in`, [fw-8] |
| `requiredACG` | Array of ACG groups for every Luna call the app makes. Not supported on webOS TV 25 or lower, enforced when present on 26 Re:New, mandatory (even empty) on 27. OSE names the same field `requiredPermissions`; flutter-webos deletes `requiredPermissions` and writes `requiredACG`. | D, D-ose | [lg-acg], [ose-appinfo], [fw] `lib/webos_sdk.dart` L136-140 |
| `trustLevel` | `default` in LG's Flutter template. A user-installed app may only be `default` or `netcast`. | D, D-3p | [fw] `appinfo.json.tmpl`, [wb] `appinfo.md` |
| `disableBackHistoryAPI` | Web only: "Disable WebAppMgr usage of the HTML5 History API". No effect on a native app. | D, D-3p | [cli] schema L193-197, [fw-12] |
| `accessibility.supportsAudioGuidance` | Default `false`; webOS TV 3.0 or higher; described through ARIA tags, so for web. Meaning for a native app: not documented. | D | [lg-appinfo] |
| `supportTouchMode`, `virtualTouch`, `supportPortraitMode`, `closeOnRotation`, `splashColor` | StanbyME model only. | D | [lg-appinfo] |
| `screenSaverProperties.preferredType`, `useGalleryMode` | 2 or 3 stretch the screensaver timeout to 30 minutes and dim; OLED TVs with webOS TV 5.0 or higher. | D | [lg-appinfo], [lg-saver] |
| `cloudgame_active` | The app reads gamepad HID itself; webOS TV 23 or higher. | D | [lg-appinfo] |
| `visible`, `transparent`, `defaultWindowType` (`card`), `uiRevision` (2), `locationHint` | Written by LG's Flutter template; `visible`, `transparent` and `defaultWindowType` are in the CLI schema. | D | [fw] `appinfo.json.tmpl`, [cli] schema L95-104, L153-167 |
| `dialAppName`, `useAllMouseButtons` (webOS 25) | Undocumented by LG. | D-3p | [wb] `appinfo.md` |
| Locale files | `resources/<locale>/appinfo.json` (for example `ko-KR`) overrides `title` and `appDescription`; `id` and `version` must equal the top-level ones; UTF-8 without BOM. The CLI scans `resources` for them. | D | [lg-l10n], [cli] `lib/package.js` L681-728 |

### 2.1 The package

| Fact | Tag | Source |
|---|---|---|
| An IPK is an `ar` archive of `debian-binary` (`2.0\n`), `control.tar.gz` and `data.tar.gz`. | D | [cli] `lib/package.js` L1555-1558, L1672-1676 |
| `data.tar.gz` holds `usr/palm/applications/<id>/` and `usr/palm/packages/<id>/packageinfo.json` with `id`, `version` and `app`. | D | [cli] `lib/package.js` L1153-1192, `files/conf/ipk.json` |
| `control` carries `Package`, `Version`, `Section: misc`, `Priority: optional`, `Architecture`, `Installed-Size`, `Maintainer`, `Description`, `webOS-Package-Format-Version: 2`, `webOS-Packager-Version`. | D | [cli] `lib/package.js` L1443-1457 |
| `Architecture` is the ELF machine of `main` (`arm`, `aarch64`, `x86_64`, `i586` for x86), or `all`. The file is named `<id>_<version>_<arch>.ipk`; LG's Flutter build produces `..._1.0.0_arm.ipk`. | D | [cli] `lib/package.js` L958-972, L1612-1620, [fw] `doc/getting-started.md` L256 |
| File names must be ASCII ("opkg does not support Posix Tar fully"). Modes come from disk; the CLI only adds directory search bits and world read. flutter-webos runs `chmod +x` on `main` first. | D | [cli] `lib/package.js` L1565-1585, [fw] `lib/webos_sdk.dart` L126-134 |
| `--sign` and `--certificate` are options of the signage profile, and hash lines go into `control` for that profile only. | D | [cli] `files/help/ares-package.help` L51-62, `lib/package.js` L1459-1469 |
| So a TV developer IPK is unsigned. | I | from the row above; LG's TV pages name no signing step |
| A package whose data entries all carry mtime 1970-01-01 is refused by firmware. | D-3p | [wb-toolbox] `common/ipk/src/lib.rs` L24-26 |

## 3. How a native app starts, lives and dies

### 3.1 Process, jail and files

| Fact | Tag | Source |
|---|---|---|
| SAM spawns one process per app and, unless the jailer is disabled or the app is on a no-jail list, runs `jailer -t <type> -i <appId> -p <appFolder> <main> <paramsJson>`. | D-ose | [sam] `NativeContainer.cpp` L155-192, [sam1] `nativeapp_life_handler.cpp` L139-162 |
| The jailer builds a private root under `/var/palm/jail`: non-root user in groups `video`, `audio`, `luna`, `compositor`, `crashd`, `se`; `/bin`, `/lib`, `/usr/lib` and `/media/internal` read only; `/media/developer` writable; few `/sys` subtrees, so udev does not work. Its config is signed and served per SDK version from `developer.lge.com`. | D-3p | [wb] `vocabulary.md`, `native/gamepad.md` |
| Developer apps live in `/media/developer/apps/usr/palm/applications/<id>`, store apps in `/media/cryptofs/apps/usr/palm/applications/<id>`. | D, D-ose, D-3p | [fw] `lib/webos_device.dart` L383, [sam] `files/conf/sam-conf.json.in`, [ose-cmake] L450-460, [wb] `filesystem.md` |
| A path starting with `/media/developer/` "may work in the Developer Mode app but will not when the app is released through LG Seller Lounge". Kodi finds its root from `/proc/self/exe`. | D, D-3p | [lg-resources], [kodi] `xbmc/platform/linux/PlatformWebOS.cpp` L32-45 |
| The launcher sets `APPID` on a TV; a shell does not. Kodi also reads `DISPLAY_ID`. | D-3p | [sdl] `src/core/webos/SDL_webos_init.c` L72, [wb] `gdb.md`, [kodi] `xbmc/windowing/wayland/ShellSurfaceWebOSShell.cpp` L86-92 |
| OSE SAM passes its own environment plus `APP_ID`, `INSTANCE_ID`, `LAUNCHPOINT_ID`, `DISPLAY_ID`, `LS2_NAME` (`<appId>-<n>`), and redirects stdout and stderr to `/var/log/<appId>-<n>`. Note `APP_ID` there, `APPID` on TV. | D-ose | [sam] `NativeContainer.cpp` L195-206 |
| Kodi states "$HOME is set to the ipk dir and $LD_LIBRARY_PATH is lib", and itself forces `XDG_RUNTIME_DIR=/tmp/xdg` and `WAYLAND_DISPLAY=wayland-0`. webosbrew says `/media/developer` is the jail's `HOME`. | D-3p | [kodi] `PlatformWebOS.cpp` L49-59, [wb] `vocabulary.md` |
| Working directory: SAM sets it to the app folder only when it launches without the jail. | D-ose | [sam] `NativeContainer.cpp` L156-158 |
| The app directory is not writable on webOS 1 and 2, and on webOS 3 until the first reboot after install. `/tmp` is writable everywhere and cleared at boot. | D-3p | [wb] `filesystem.md` |
| LG's Flutter tool creates `<app>/appdata` with mode 777 inside the package and puts home, `.cache`, `.data`, `.download` and `log` under it. An absolute home "must be under the /media/developer/temp/ directory". | D | [fw] `lib/webos_sdk.dart` L142-149, `templates/app/meta/flutter-conf.json`, `lib/webos_device.dart` L188-201 |
| So LG does not count on the app user owning its install directory, only on a world-writable folder shipped in the package. | I | from the row above |
| System logs are closed to `prisoner` on a retail TV; LG's tool tails a log file the app writes under `appdata/log` through `ares-novacom --run`. | D, D-3p | [fw] `lib/webos_device.dart` L377-401, `lib/webos_ares.dart` L193-198, [fw-8] |

### 3.2 Launch arguments

| Fact | Tag | Source |
|---|---|---|
| The last argument SAM passes is one JSON string. | D-ose | [sam] `NativeContainer.cpp` L159, L190 |
| Version 1: the launch `params` object itself, plus `"nid": <appId>` and `"@system_native_app": true` (and `preload`). | D-ose | [sam1] `nativeapp_life_handler.cpp` L282-298 |
| Version 2: `{"event":"launch","reason":...,"appId":...,"interfaceVersion":2,"interfaceMethod":"registerApp","parameters":{...},"@system_native_app":true}`. | D-ose | [sam1] `nativeapp_life_handler.cpp` L451-467 |
| Today's OSE SAM merges both: the params object plus `event`, `reason`, `appId`, `nid`, `@system_native_app`. | D-ose | [sam] `NativeContainer.cpp` L115-120 |
| That the TV's jailer hands this string to `main` unchanged as `argv[1]`. | I | from the command lines above; the jailer is closed |
| `params` come from `launch`: `{"id": "<appId>", "params": {...}}`, called by the launcher, another app, a service or `ares-launch`. | D | [lg-appmgr], [lg-lcm] |
| LG's public pages name no launch parameter for a deep link. Its events page says of `webOSLaunch` only "This event is fired when an app is launched", and staff answer questions about deep links with "webOS TV partner resources are protected by a non-disclosure agreement". | D | [lg-events], [forum-24869], [forum-28146] |
| OSE's app manager documents the parameter: for the YouTube app, `"params":{ "contentTarget" : "https://www.youtube.com/tv?v=9bZkp7q19f0"}`. | D-ose | [ose-sam-api], `launch` |
| On TVs, LG's Home and Search open a card or a search result by launching the app with the `deeplinkingParams` of its `appinfo.json`, `$CONTENTID` replaced: `{"contentTarget":"$CONTENTID"}` for Netflix, Prime Video and YouTube (Disney+ adds `"handledBy":"app"`). An app with no template gets a bare `contentTarget`, and so does the remote control service's `contentId`. Seen on a G3 (webOS 9.2.2), a 50QNED87T6B (10.2.2) and a 43UM7100PLB (4.10.2). | D-3p | [glasshouse] `docs/development/TV-RESEARCH.md` L1042-1049, L1098-1101, L1868-1871, L1887-1893 |
| So `contentTarget` is the deep link of a launch, and it may be a content id of the app's own as well as a URL. | I | from the three rows above |

### 3.3 Registering with SAM

`luna://com.webos.applicationManager/...`, also served as
`com.webos.service.applicationManager` and `com.webos.service.applicationmanager`
([sam] `files/sysbus/com.webos.sam.api.json`). D-ose

| | Version 1 (default) | Version 2 |
|---|---|---|
| Method | `registerNativeApp` | `registerApp` |
| Wrong method for the declared version | error "trying to register via unmatched method with nativeLifeCycleInterfaceVersion" | same |
| First reply | `{"returnValue":true,"message":"registered"}` | `{"returnValue":true,"event":"registered"}` |
| Launched again while running | `{"message":"relaunch","parameters":{...},"returnValue":true}` | `{"event":"relaunch","reason":...,"appId":...,"parameters":{...},"returnValue":true}` |
| Close | SIGTERM to the process and its children, SIGKILL after 1 s | `{"event":"close","reason":...}`, SIGKILL after 10 s, or after 1 s when memory is being reclaimed; no SIGTERM |
| Pause (kept alive in background) | SIGTERM, SIGKILL after 1 s | `{"event":"pause","reason":"keepAlive","parameters":{...}}` |
| Running but not registered | launched again within 3 s: ignored; later: SIGTERM, then a new process | not registered 3 s after start and launched again, or closed: SIGKILL |

Source: [sam1] `nativeapp_life_handler.cpp` L34-37, L312-317, L333-430,
L473-581, L831-862; `src/core/lifecycle/app_life_manager.cpp` L789-850;
`src/core/bus/lifecycle_luna_adapter.cpp` L91-94, L492-525. D-ose

| Fact | Tag | Source |
|---|---|---|
| Both versions exist on TVs: SDL-webOS reads `nativeLifeCycleInterfaceVersion` through `getAppInfo` (`{"id": <appId>}`), then calls `registerNativeApp` and reads `message`, or `registerApp` and reads `event`. Its payload is `{"id": <APPID>}`. | D-3p | [sdl] `SDL_webos_init.c` L135-241 |
| Kodi registers version 2 at `luna://com.webos.service.applicationmanager/registerApp` and on `relaunch` goes fullscreen again. | D-3p | [kodi] `xbmc/windowing/wayland/WinSystemWaylandWebOS.cpp` L35, L196-209 |
| SAM finds the caller by the application id of the message, else by the sender's service name. | D-ose | [sam1] `src/core/base/lsutils.cpp` L65-74 |
| The registration is a subscription that stays open: every later event is one more reply to it. | D-ose | [sam1] `nativeapp_life_handler.cpp` L667-688 |
| An app has one registration. The first OSE app manager replaces it when the app registers again: the earlier call gets no further event and is not told. Today's refuses the second call with "The app is already registered". Which of the two a TV does is not known. | D-ose | [sam1] `nativeapp_life_handler.cpp` L606-624, L831-843; [sam] `src/base/RunningApp.cpp` L154-160 |
| A `relaunch` carries the caller's `params` as `parameters`: `{"message":"relaunch","parameters":{...},"returnValue":true}` under interface 1, and with `event`, `reason` and `appId` under interface 2. | D-ose | [sam1] `nativeapp_life_handler.cpp` L312-317, L473-480 |
| Started from a shell, without `APPID`, an SDL app fails to register without an error at the call, and its window is never created. | D-3p | [wb] `gdb.md`, [sdl] `SDL_waylandwebos.c` L139-142 |
| Today's OSE SAM: both method names reach one handler; events are `registered`, `relaunch`, `pause`, `close` with `returnValue`, `subscribed`, `event`, `appId`, `parameters`, `reason`; `registerApp` belongs to ACG `application.operation`. | D-ose | [sam] `src/bus/service/ApplicationManager.cpp` L164-165, L412-441, `src/base/RunningApp.h` L272-280, [ose-sam-api] |
| Today's OSE SAM closes a registered app with the `close` event and an unregistered one with SIGTERM to its process group; either way SIGKILL follows after 1 s and repeats. Other transitions get 10 s. | D-ose | [sam] `PolicyManager.cpp` L78-103, `src/base/RunningApp.cpp` L243-255, L264-280, `RunningApp.h` L310-311, `src/util/NativeProcess.cpp` |

### 3.4 Foreground and background

| Fact | Tag | Source |
|---|---|---|
| A web app is Not Launched, Launched (foreground) or Suspended (background), and sees `webOSLaunch`, `webOSRelaunch` and `visibilitychange`. | D | [lg-lifecycle], [lg-lcm] |
| A native app learns the same from its Wayland shell surface: state `MINIMIZED` is background, `FULLSCREEN` is foreground, `state_about_to_change` comes first, `close` asks it to quit. | D-3p | [sdl] `src/video/wayland/SDL_waylandwebos.c` L183-231 |
| Clients put the app id on the surface as property `appId` with `wl_webos_shell_surface_set_property`. | D-ose, D-3p | [ose-native-tut], [sdl] `SDL_waylandwebos.c` L144 |
| `pause`: "Triggered when transitioning the foreground app to the background. The close event is delivered only when memory manager terminates." | D-ose | [ose-sam-api] |
| Users close an app with the X button of the system UI or by holding Back; apps "are not allowed to have its own close button". | D | [lg-lcm], [lg-interact] |

### 3.5 Back and Exit

| Fact | Tag | Source |
|---|---|---|
| Web rule: Back goes to the previous page; on the entry page, webOS TV 6.0 or higher shows an exit popup, 5.0 or lower opens Home. `webOS.platformBack()` does that for the app. | D | [lg-back] |
| The checklist differs: Back on the entry page "must display the Home screen (webOS23 - webOS25 only)". | D | [lg-checklist] |
| A native app never sees Back unless its surface sets `_WEBOS_ACCESS_POLICY_KEYS_BACK` to `"true"`. `_EXIT` (long press on Back), `_HOME`, `_GUIDE` and `_META` work the same and default to off. | D-3p | [sdl] `SDL_waylandwebos.c` L145-158, `include/SDL_hints.h` L2714-2724, [wb] `native/remote.md` |
| On webOS 26, a Flutter app gets no Back key: each press opens the system exit popup (`com.webos.app.alert`). LG: the SDK "does not yet support apps intercepting the back key", with an update planned for the end of 2026. | D-3p, D | [fw-12] |

## 4. Luna bus from a native process

### 4.1 luna-service2 C API

```c
bool LSRegister(const char *name, LSHandle **sh, LSError *lserror);
bool LSGmainAttach(LSHandle *sh, GMainLoop *mainLoop, LSError *lserror);
bool LSGmainContextAttach(LSHandle *sh, GMainContext *mainContext, LSError *lserror);
bool LSCall(LSHandle *sh, const char *uri, const char *payload,
            LSFilterFunc callback, void *user_data,
            LSMessageToken *ret_token, LSError *lserror);
bool LSCallFromApplication(LSHandle *sh, const char *uri, const char *payload,
            const char *applicationID, LSFilterFunc callback, void *ctx,
            LSMessageToken *ret_token, LSError *lserror);
bool LSCallCancel(LSHandle *sh, LSMessageToken token, LSError *lserror);
const char *LSMessageGetPayload(LSMessage *message);
```

| Fact | Tag | Source |
|---|---|---|
| Signatures above; `LSCallOneReply` has the signature of `LSCall`. | D-ose | [ls2] `include/public/luna-service2/lunaservice.h` L357-361, L422, L449-450, L509-554 |
| `LSRegister(NULL, ...)` registers a nameless client. Replies arrive on the GLib loop or context the handle is attached to. | D-ose | [ose-ls2] |
| A subscription is an `LSCall` whose payload has `"subscribe": true`; it lives until `LSCallCancel`. `LSCallOneReply` "only accepts one response and does not need to be canceled". | D-ose | [ose-ls2] |
| `libluna-service2.so.3` exports all of these in every firmware sample from webOS 1.2 to 11.2 (library 3.6.0 to 3.21.2). `LSRegisterApplicationService` appears in 3.x; `LSRegisterPubPriv` is gone in 11.2. | I | computed from [wb-fwdata] |
| Homebrew ports do not call it directly: they load LG's `libhelpers.so.2` and use `HLunaServiceCall(uri, payload, HContext *)`, whose context has `callback`, `multiple` (subscription) and `pub` (public bus). The library is in every sample, 1.2 to 11.2. | D-3p, I | [wu] `include/webos-helpers/libhelpers.h`, [sdl] `src/core/webos/SDL_webos_libs.c` L71; presence computed from [wb-fwdata] |
| "HLunaServiceCall() hangs for good when libhelpers can't make its luna bridge, e.g. for a binary without an ls2 role": it retries under a lock it already holds. SDL makes the bridge first, by calling `HLunaServiceBridge::instance()` (`_ZN18HLunaServiceBridge8instanceEv`), and gives up Luna calls when that returns null. | D-3p | [sdl3] `src/core/webos/SDL_webos_libs.c` L90-116 |
| libhelpers starts a GLib main loop thread of its own on first use, delivers every reply on it, and cannot stop it, so the library is never unloaded. | D-3p | [sdl3] `SDL_webos_libs.c` L49-51, `SDL_webos_luna.c` L114-116 |

### 4.2 Identity and permissions

| Fact | Tag | Source |
|---|---|---|
| In OSE every bus client has a role file (`exeName` or `appId`, `allowedNames`, `trustLevel`) and a client permission file listing ACG groups; an app's groups come from `requiredPermissions`. | D-ose | [ose-sec] |
| A JS service name must begin with the app id. | D | [lg-appinfo] |
| OSE's native sample registers with `luna-send -a <appId>`; today's OSE SAM expects the name `<appId>-<n>` it put in `LS2_NAME`, then falls back to the app id. | D-ose | [sam] `apps/com.webos.app.sam-native-registered/main.sh`, `ApplicationManager.cpp` L421-425 |
| Which name a jailed TV app may register, and whether the system writes its role file at install: not documented. | | open question |
| A call outside the declared groups fails with `{"errorCode":-1,"returnValue":false,"errorText":"Denied method call \"{method}\" for category \"/\""}`. | D | [lg-acg] |
| flutter-webos always adds `lunabus.query`, `application.launcher`, `settings.query`, `systemconfig.query` and `tts.operation`. | D | [fw] `lib/webos_sdk.dart` L76-82 |
| LG marks no public Luna API as store-only. Limits that are documented: `launch` error -104 "Cannot launch a privileged app on development mode from developer apps path"; deep links need a partnership. | D | [lg-appmgr], [seller-faq] |

### 4.3 Services a TV app uses

ACG is the group to declare from webOS TV 26 Re:New ([lg-acg]). LG lists
Application Manager, Connection Manager, Settings Service and System Service as
supported on webOS TV 1.x to 26, TV Device Information as partial on 1.x and
Device Unique ID from 3.x ([lg-luna]). D

| Need | Call and payload | Reply | ACG | Source |
|---|---|---|---|---|
| Model, firmware, version | `luna://com.webos.service.tv.systemproperty/getSystemInfo` `{"keys":["modelName","firmwareVersion","sdkVersion","UHD","boardType"]}` | each key as a string: `{"returnValue":true,"modelName":"WEBOS1","firmwareVersion":"3.00.00","UHD":"true","sdkVersion":"1.05.00"}`; `boardType` is `<chipset>_<standard>_<country>`. "If you used a wrong key or error occurs, no result data will be returned for that key." A failure is `{"returnValue":false,"errorCode":"ERROR_06","errorText":"Invalid argument."}` | `systemconfig.query` | D [lg-sysprop] |
| Language, country | `luna://com.webos.settingsservice/getSystemSettings` `{"keys":["localeInfo"]}` or `{"category":"option","keys":["country","smartServiceCountryCode2"]}`; `subscribe` allowed | `{"method":"getSystemSettings","settings":{"localeInfo":{"locales":{"UI":"en-GB","TV":"en-GB","FMT":"en-GB","NLP":"en-GB","STT":"fr-FR","AUD":"fr-FR","AUD2":"en-GB"},"clock":"locale","keyboards":["en"],"timezone":""}},"returnValue":true}`; `country` alpha-3, `smartServiceCountryCode2` alpha-2. An unknown key answers `errorText` "There is no matched result from DB." | `settings.query` | D [lg-settings] |
| Audio guidance (the screen reader) | the same call with `{"category":"option","keys":["audioGuidance"]}`; webOS TV 3.0 or higher | `{"subscribed":false,"category":"option","method":"getSystemSettings","settings":{"audioGuidance":"off"},"returnValue":true}` | `settings.query` | D [lg-settings] |
| The name the viewer gave the TV | LG documents no key for it: the settings keys are `localeInfo`, `country`, `smartServiceCountryCode2`, `audioGuidance` and the caption settings | | | D [lg-settings] |
| Brand, manufacturer | no call: LG's web library reports `brandName` "LG" and `manufacturer` "LG Electronics" as constants beside `sdkVersion`, "The webOS TV SDK version in X.X.X format" | | | D [lg-webosjs] |
| Network | `luna://com.palm.connectionmanager/getStatus` `{"subscribe":true}`. On webOS TV 3.x subscribe through `luna://com.webos.service.connectionmanager`, the name LG's Flutter plugins use | `isInternetConnectionAvailable`, `wired` and `wifi` with `state`, `ipAddress`, `onInternet` | `network.query` | D [lg-conn], [fwp] `packages/webos_service_bridge/README.md` |
| Device id | `luna://com.webos.service.sm/deviceid/getIDs` `{"idType":["LGUDID"]}`; webOS 3.0 or later; needs the user's consent | `idList: [{"idType":"LGUDID","idValue":"..."}]` | `deviceinfo.query` | D [lg-devid] |
| Time zone | `luna://com.palm.systemservice/time/getSystemTime` | `utc`, `localtime`, `offset`, `timezone`, `TZ` | `time.query` | D [lg-time] |
| Open a URL or app | `luna://com.webos.applicationManager/launch` `{"id":"com.webos.app.browser","params":{"target":"https://..."}}` | `returnValue`, or `errorCode` (-101 app not found) | `application.launcher` | D [lg-appmgr], [lg-acg]; D-3p [sdl] `src/misc/webos/SDL_sysurl.c` L27-43 |
| Own appinfo | `luna://com.webos.applicationManager/getAppInfo` `{"id":"<appId>"}` | `appInfo` | not listed | D-3p [sdl] `SDL_webos_init.c` L143-160 |
| Screen saver | LG documents no call: the saver stays off during full screen video, and `screenSaverProperties` stretches it. Homebrew subscribes to `luna://com.webos.service.tvpower/power/registerScreenSaverRequest` `{"subscribe":true,"clientName":"<id>.wakelock"}` and answers each event at `.../responseScreenSaverRequest` with `{"clientName":...,"ack":false,"timestamp":<same>}` to refuse, `"ack":true` to let it start | events carry `timestamp`; one without is not a request | not listed | D [lg-saver]; D-3p [sdl] `SDL_webos_init.c` L243-304, [sdl3] `SDL_webos_init.c` L282-345 |
| Panel size | `luna://com.webos.service.panelcontroller/getPanelResolution` `{}` | `width`, `height` | not listed | D-3p [sdl] `src/core/webos/SDL_webos_panelinfo.c` L11-52 |

## 5. Model year, version and ABI

LG's table gives the platform per release year; "The release year refers to the
release year of each platform version, not the TV release year" ([forum-8552],
LG staff). The library columns are the file names and symbol versions in
webosbrew's firmware symbol data ([wb-fwdata], one or two firmware images per
release), so they are D-3p for the image named in the last column.

| Year | webOS TV | Release, code name | Web engine | glibc | libstdc++ (first GCC with it) | ICU | Kernel | Firmware sample |
|---|---|---|---|---|---|---|---|---|
| 2014 | 1.x | 1.x `afro` | WebKit 537.41 | 2.14.1 | 6.0.16 (4.6.1) | 50.1.2 | ? | 1.2.0 `WT1M`, 1.4.0 `WT1H` |
| 2015 | 2.x | 2.x `beehive` | WebKit 538.2 | 2.19 | 6.0.19 (4.8.3) | 50.1.2 | ? | 2.2.3 `W15M` |
| 2016 | 3.x | 3.0 to 3.4 `dreadlocks` | Chromium 38 | 2.19 | 6.0.19 (4.8.3) | 53.1 | ? | 3.4.0 `W16N` |
| 2017 | 3.x | 3.5 to 3.9 `dreadlocks2` | Chromium 38 | 2.19 | 6.0.19 (4.8.3) | 53.1 | ? | 3.9.2 `W17H` |
| 2018 | 4.x | 4.0 to 4.4 `goldilocks` | Chromium 53 | 2.24 | 6.0.22 (6.1) | 57.1 | ? | 4.4.2 `W18R` |
| 2019 | 4.x | 4.5 to 4.10 `goldilocks2` | Chromium 53 | 2.24 | 6.0.22 (6.1) | 57.1 | ? | 4.10.0 `W19P` |
| 2020 | 5.x | 5.x `jhericurl` | Chromium 68 | 2.28 | 6.0.25 (8.1) | 62.1 | ? | 5.3.1 `W20L` |
| 2021 | 6.x | 6.x `kisscurl` | Chromium 79 | 2.28 | 6.0.25 (8.1) | 62.1 | ? | 6.4.0 `W21P` |
| 2022 | 22 | 7.x `mullet` | Chromium 87 | 2.31 | 6.0.28 (9.3) | 66.1 | 5.4.96 | 7.4.0 `W22O` |
| 2023 | 23 | 8.x `number1` | Chromium 94 | 2.31 | 6.0.28 (9.3) | 66.1 | 5.4.268 on a C3, aarch64 | 8.3.0 `W23O` |
| 2024 | 24 | 9.x `ombre` | Chromium 108 | 2.35 or later | 6.0.29 (11.1) | 70.1 | ? | 9.2.0 on a `W23O` |
| 2025 | 25 | 10.x `ponytail` | Chromium 120 | 2.35 or later | 6.0.29 (11.1) | 70.1 | ? | 10.2.0 on a `W22H` |
| 2026 | 26 | 11.x `queue` | Chromium 132 | 2.39 or later | 6.0.32 (13.2) | 74.2 | 6.12.0, aarch64 | 11.2.0 on a `W23O` |
| 2027 | 27 | `spacebuns` | ? | ? | ? | ? | ? | none |

- **Year, name, engine.** [lg-engine] (it gives 2016~2017 for 3.x and 2018~2019
  for 4.x). D. Release numbers and code names: [wb] `library-version.md`;
  `queue` is 26 and `spacebuns` 27 in [fw] `lib/webos_sdk.dart` L53-56. The
  year split of 3.x and 4.x follows the OTA ids of the samples. D-3p, D, I
- **OS upgrades.** The same database lists 2022 models (`mullet`) with firmware
  variants on webOS 23, 24, 25 and 26, and 2023 models up to 26 ([wb-caniroot]
  `src/models.gen.js`). D-3p. A model year therefore names the oldest webOS a
  TV can run, not the one it runs. I
- **glibc.** `ld-<version>.so` and `libc-<version>.so` up to 8.x. From 9.x the
  files are unversioned, so the cell is the highest `GLIBC_2.x` symbol version
  `libc.so.6` defines. I (computed from [wb-fwdata]). A 2022 C2 owner reports
  2.31 and GCC 9 ([forum-2599]). D-3p
- **GCC.** [gcc-abi] maps `libstdc++.so.6.0.x` to the first GCC that shipped
  it; 6.0.28 is both 9.3.0 and 10.1.0. D. What LG compiles with: not published.
- **Kernel.** 5.4.96 "found on webOS 7 devices" ([kodi] `docs/README.webOS.md`
  L4) and on a C2 ([forum-2599]); `5.4.268-320 ... aarch64` on a 2023 C3 whose
  webOS version is not stated ([forum-27101]); `6.12.0-61.qausui.8` on a 2026
  43NU850B6LA with webOS TV Lite 11.0.1 ([fw-8]). D-3p. Other cells: no source.
- **Userland is 32-bit ARM on every release.** LG's 2026 NDK is
  `webos-ndk-flutter-starfish-x86_64-ca9v1-11.2.0`, its environment
  `ca9v1-webosmllib32-linux-gnueabi`, the tool's default compiler prefix
  `arm-starfishmllib32-linux-gnueabi-` and target arch `arm` ([fw]
  `doc/getting-started.md` L91-98, `lib/webos_sdk.dart` L27-32). D. Firmware
  packages are named `lib32-*` from 7.x on ([wb-fwdata]). D-3p. On a C3:
  kernel `aarch64`, `getconf LONG_BIT` 32 ([forum-27101]); on the 2026 TV:
  `uname -m` `aarch64`, "32-bit userland (`lib32`)", image
  `lib32-starfish-global-secured` ([fw-8]). D-3p
- **Float ABI is softfp.** The loader is `/lib/ld-linux.so.3`, never
  `ld-linux-armhf.so.3`, in all samples (computed from [wb-fwdata]). I. A
  working Flutter binary is "ELF32 ARM, `Version5 EABI, soft-float`, matching
  the TV's `/lib/ld-linux.so.3`" ([fw-8]). The homebrew toolchain is
  `arm-webos-linux-gnueabi`: `cortex-a9`, `neon-fp16`, `softfp`, kernel headers
  3.10, glibc 2.12.2 with polyfills, GCC 16.2 ([nc4] `configs/webos_tv_defconfig`
  L45-52, L273, L308, [nc4-glibc]). Kodi builds with
  `-march=armv7-a -mfloat-abi=softfp -mfpu=neon` ([kodi] `docs/README.webOS.md`
  L141). D-3p
- **Library names.** `libEGL.so.1`, `libGLESv2.so.2`, `libwayland-client.so.0`,
  `libwayland-webos-client.so.1`, `libglib-2.0.so.0`, `libluna-service2.so.3`,
  `libpbnjson_c.so.2`, `libPmLogLib.so.3` and `libhelpers.so.2` resolve in
  every sample, 1.2 to 11.2. These move: `libwayland-egl.so.0` up to 4.4,
  `.so.1` from 4.10; `libcurl.so.5` up to 6.x, `libcurl.so.4` from 5.x; OpenSSL
  1.0.0 up to 4.x, 1.0.2 in 5.x and 6.x, 1.1 in 6.x to 8.x, 3 from 9.x.
  Computed from each sample's `index.json` in [wb-fwdata]. I. [wb]
  `native/net.md` says the same of curl and OpenSSL. D-3p
- **webOS extensions to Wayland change opcodes between releases without a
  version bump** ([wb] `wayland.md`). D-3p. See the rendering reference.

## 6. Screen

| Fact | Tag | Source |
|---|---|---|
| Graphics: 1280x720 on Full HD models, 1920x1080 on Ultra HD models. Video: 1920x1080 and 3840x2160. | D | [lg-res] |
| The default graphics resolution is 1920x1080; `resolution` in `appinfo.json` selects 1280x720. LG recommends one package per resolution. | D | [lg-res], [lg-appinfo] |
| "webOS TV does not support UHD resolution for web apps." | D | [lg-appinfo] |
| For a native Wayland client too: "The reported resolution is always 1080p even for 4K devices and 720p for HD devices". | D-3p | [kodi] `WinSystemWaylandWebOS.cpp` L211-214 |
| SDL-webOS sets the surface property `_WEBOS_ACCESS_POLICY_FORCESTRETCH` to `"true"`. `wl_webos_shell_surface` gains a `set_size` request in webOS 10.2. What either does to the surface size: not documented. | D-3p | [sdl] `SDL_waylandwebos.c` L179, [wb] `wayland.md` |
| The panel's own size comes from `getPanelResolution`, or from `UHD` of `getSystemInfo`. | D-3p | [sdl] `SDL_webos_panelinfo.c` L11-52 |
| 8K models exist: machine names `o208k`, `o228k`, `o22n8k` with OTA ids `W20K`, `W22K`, `W23K`. | I | from the names in [wb-caniroot] `tools/mappings.js` L111-132 |
| Safe area: "a 20 px margin area around the edges" (described for webOS TV 6.0); no selectable object, text or logo outside it. | D | [lg-overscan] |
| Recommended minimums at 1920x1080: 75x75 px buttons, 20 px text; at 1280x720: 50x50 px and 14 px. | D | [lg-checklist] |

## Open questions

1. Does Seller Lounge accept a `type: "native"` IPK from a non-partner, and
   since which webOS? Only LG can answer (1:1 QA or the Partner Zone).
2. Does the retail jailer pass the launch JSON as `argv[1]`, and which of
   `HOME`, `XDG_RUNTIME_DIR`, `WAYLAND_DISPLAY`, `LD_LIBRARY_PATH` and the
   working directory does it set? Kodi and webosbrew disagree on `HOME`.
3. Which directory survives an app update, and which survives nothing? Is
   `<app>/appdata` kept when a new IPK is installed over the old one?
4. May a jailed app call `LSRegister` with its app id, and on which releases?
   What does `libhelpers.so.2` register as? Does webOS 26 still let a
   developer-mode native app call `registerApp` with only flutter-webos's
   default ACG list, and is `tvpower` reachable under ACG?
5. flutter-webos documents both `requiredACG` in `appinfo.json` and a separate
   permission file "installed to the correct system paths" ([fw]
   `doc/getting-started.md` L349-369). Which one does a TV read?
6. What should a native app do on Back at its root: exit, minimize, or ask the
   system for the exit popup? LG's rule differs by version (3.5) and LG's own
   Flutter runtime cannot intercept Back yet.
7. Does `handlesRelaunch` change anything for a native app on a TV?
   Does a TV's app manager send `relaunch` with `parameters` to a registered
   native app, and does it replace or refuse a second registration (3.3)?
   Does an app need `deeplinkingParams` in its `appinfo.json` for LG's Home and
   Search to hand it a `contentTarget` (3.2)?
8. Is "webOS 26 Re:New" firmware 11.2 and later? The NDK is 11.2.0 and a
   stock Flutter app showed no window on a retail 11.0.1 "webOS TV Lite"
   ([fw-8], unanswered).
9. Kernel versions before webOS 22, and whether any TV ships a 64-bit
   userland. The 11.2.0 symbol index names `ld-linux-aarch64.so.1` without
   data for it ([wb-fwdata]).
10. Can a native surface be 3840x2160 on any release (`set_size`, 10.2)?
11. Memory limit per app and the meaning of `requiredMemory` on a TV.
12. Developer Mode limits LG does not publish: session length (1000 hours is
    third party), number of apps, size of `/media/developer` per model.

## What this means for react-native-webos

1. **Distribution is Developer Mode and homebrew until LG says otherwise.** No
   .NET shell trick exists or is needed: the IPK holds an ELF and SAM executes
   it. A store route is a partnership question (open question 1). Record the
   decision in `docs/decisions.md` before any doc promises LG Apps.
2. **One ABI: 32-bit `armv7-a`, `softfp`, NEON, loader `/lib/ld-linux.so.3`.**
   Every firmware sample loads through it, none has a hard-float loader, and
   LG's own 2026 NDK builds 32-bit. The simulator must offer the same ABI,
   unlike the Tizen one.
3. **The floor is a glibc number.** Link libstdc++ statically (the TV's is GCC
   4.6 to 13) and cap glibc symbols: 2.28 reaches webOS 5 (2020), 2.24 webOS 4
   (2018), 2.19 webOS 2 (2015). LG's NDK targets webOS 26 and is only known to
   work there; the openlgtv toolchain (glibc 2.12.2) reaches every release.
   Bundle curl, OpenSSL and ICU, whose names move, and mind `libwayland-egl`
   below 4.10. Check each build against [wb-fwdata] the way
   `tizen-tv-platform.md` 2.1 checks rootstraps.
4. **Write `appinfo.json` from the Expo config** with `type: "native"`,
   `main`, `nativeLifeCycleInterfaceVersion: 2`, `handlesRelaunch: true`,
   80x80 and 130x130 icons, a 1920x1080 `splashBackground`, `iconColor`, and
   `requiredACG` built from the modules in use (start from flutter-webos's
   five). Validate `id` and the three-part `version` with LG's rules.
5. **Package without `ares-package` if we want to**: `ar` of `debian-binary`,
   `control.tar.gz`, `data.tar.gz`; `Architecture: arm`; real mtimes; ASCII
   names; the exec bit on `main`; a world-writable `appdata/` if we follow LG.
6. **Lifecycle lives in the host, not in a shell.** Parse the launch JSON,
   call `registerApp` within 3 s of start, map `relaunch` parameters to
   `Linking`, `pause` and the surface state to `AppState`, and exit within 1 s
   of `close`. Handle SIGTERM too: it is what an unregistered or version 1
   process gets. The host makes the one registration an app has, in SDL's
   place (`webos-window-input.md`, section 15). The deep link of a launch or a
   relaunch is its `contentTarget`: `Linking.getInitialURL` answers the first,
   a `url` event delivers the others, and `webOSRelaunch` carries every
   relaunch's parameters whole.
7. **The host needs a GLib main context** for luna-service2 replies, on every
   release, next to the Wayland loop.
8. **Back is opt-in**: set `_WEBOS_ACCESS_POLICY_KEYS_BACK`, feed
   `BackHandler`, and settle the root behaviour (open question 6).
9. **Find paths at run time** (`/proc/self/exe`, `APPID`), never
   `/media/developer/...`; set `XDG_RUNTIME_DIR` and `WAYLAND_DISPLAY` when
   they are missing; do not write next to the binary below webOS 4.
10. **Keep the one logical 1920x1080 screen.** It is the surface LG gives on
    UHD panels; on Full HD models the surface is 1280x720, so the host scales.
11. **Logs travel over the network or a file under `appdata/`**, as in
    react-native-tizen: system logs are closed on retail TVs. The host does
    not write to PmLogLib for the same reason: nobody with a retail TV can
    read what it would log (3.1).
12. **Budget the binary**: under 1 GB of developer storage on small models.
13. **The device, as JavaScript sees it**, comes from one `getSystemInfo` and
    one `getSystemSettings` sent before the window opens and waited for at most
    half a second in all. `Platform.constants` has `Model` (`modelName`),
    `Release` (`sdkVersion`, the webOS TV platform version), `Fingerprint`
    (`firmwareVersion`), `Brand` "LG", `Manufacturer` "LG Electronics" and
    `uiMode` "tv". `Platform.Version` is `sdkVersion` as a number, the major
    plus a hundredth per minor (10.02 for `10.2.0`, 4.1 for `4.10.0`), since
    minors reach two digits. The menu language is `I18nManager`'s
    `localeIdentifier` and ICU's default locale. The TV's name has no
    documented source: `deviceName` is the model.

## Sources

Local: `@webos-tools/cli` 3.2.5 (`[cli]`, Apache-2.0), as installed by
`bun add -g @webos-tools/cli` under
`~/.bun/install/global/node_modules/@webos-tools/cli`. Repositories were read
from shallow clones at the commits below.

[lg-appinfo]: https://webostv.developer.lge.com/develop/references/appinfo-json
[lg-devmode]: https://webostv.developer.lge.com/develop/getting-started/developer-mode-app
[lg-lifecycle]: https://webostv.developer.lge.com/develop/getting-started/app-lifecycle
[lg-lcm]: https://webostv.developer.lge.com/develop/guides/app-lifecycle-management
[lg-webtypes]: https://webostv.developer.lge.com/develop/getting-started/web-app-types
[lg-resources]: https://webostv.developer.lge.com/develop/getting-started/app-resources
[lg-discover]: https://webostv.developer.lge.com/discover
[lg-back]: https://webostv.developer.lge.com/develop/guides/back-button
[lg-interact]: https://webostv.developer.lge.com/develop/guides/interacting-with-apps
[lg-overscan]: https://webostv.developer.lge.com/develop/guides/overscan
[lg-saver]: https://webostv.developer.lge.com/develop/guides/screensaver
[lg-l10n]: https://webostv.developer.lge.com/develop/guides/app-localization
[lg-acg]: https://webostv.developer.lge.com/develop/guides/acg-guide
[lg-flutter]: https://webostv.developer.lge.com/develop/guides/flutter-for-webos
[lg-flutter-news]: https://webostv.developer.lge.com/news/flutter-for-webos-tv-is-now-available-for-developers
[lg-flutter-2024]: https://webostv.developer.lge.com/news/2024-07-15-new-and-successful-experiment-of-webos-with-flutter
[lg-res]: https://webostv.developer.lge.com/develop/specifications/app-resolution
[lg-engine]: https://webostv.developer.lge.com/develop/specifications/web-api-and-web-engine
[lg-luna]: https://webostv.developer.lge.com/develop/references/luna-service-introduction
[lg-appmgr]: https://webostv.developer.lge.com/develop/references/application-manager
[lg-sysprop]: https://webostv.developer.lge.com/develop/references/tv-device-information
[lg-events]: https://webostv.developer.lge.com/develop/references/webos-event
[lg-webosjs]: https://webostv.developer.lge.com/develop/references/webostvjs-webos
[lg-settings]: https://webostv.developer.lge.com/develop/references/settings-service
[lg-conn]: https://webostv.developer.lge.com/develop/references/connection-manager
[lg-devid]: https://webostv.developer.lge.com/develop/references/device-unique-id
[lg-time]: https://webostv.developer.lge.com/develop/references/system-service
[lg-approval]: https://webostv.developer.lge.com/distribute/app-approval-process
[lg-eco]: https://webostv.developer.lge.com/distribute/app-ecosystem
[lg-checklist]: https://webostv.developer.lge.com/distribute/app-self-checklist
[lg-contact]: https://webostv.developer.lge.com/contact
[lg-partner]: https://webospartner.developer.lge.com/
[seller-faq]: https://seller.lgappstv.com/seller/support/faq/RetrieveFaq.lge
[forum-3365]: https://forum.webostv.developer.lge.com/t/native-language-c-c-support-issues/3365
[forum-2599]: https://forum.webostv.developer.lge.com/t/please-improve-and-update-your-base-packages/2599
[forum-8552]: https://forum.webostv.developer.lge.com/t/are-the-documented-model-year-web-os-version-couplings-reliable/8552
[forum-27101]: https://forum.webostv.developer.lge.com/t/time-to-switch-your-toolchains-to-64-bit/27101
[forum-28079]: https://forum.webostv.developer.lge.com/t/request-for-early-access-to-lg-flutter-sdk-for-webos-tv-app-development/28079
[forum-28525]: https://forum.webostv.developer.lge.com/t/28525
[forum-24869]: https://forum.webostv.developer.lge.com/t/how-to-implement-deep-linking-and-add-content-to-lg-webos-tv/24869
[forum-28146]: https://forum.webostv.developer.lge.com/t/how-to-implement-deep-linking-in-webos-tv-app-so-content-from-lg-home-search-opens-and-plays-movie-automatically/28146
[lg-sim]: https://webostv.developer.lge.com/develop/tools/simulator-installation
[lg-sim-intro]: https://webostv.developer.lge.com/develop/tools/simulator-introduction
[seller-main]: https://seller.lgappstv.com/seller/main/Main.lge
[seller-cloudlab]: https://seller.lgappstv.com/seller/support/userGuide/RetrieveUserGuide.lge?pstgSeq=49
[seller-upload]: https://seller.lgappstv.com/seller/support/userGuide/RetrieveUserGuide.lge?pstgSeq=20
[seller-alpha]: https://seller.lgappstv.com/seller/support/userGuide/RetrieveUserGuide.lge?pstgSeq=23
[tvlabs]: https://tvlabs.ai/
[tvlabs-webos]: https://docs.tvlabs.ai/platform/platforms/webos
[tvlabs-sideload]: https://docs.tvlabs.ai/platform/guides/sideloading
[ose-native]: https://www.webosose.org/docs/tutorials/native-apps/developing-downloadable-native-apps/
[ose-2.20]: https://www.webosose.org/blog/2023/02/17/webos-ose-2-20-0-release/
[fw]: https://github.com/lg-flutter-webos/flutter-webos/tree/950398131830e9bd5dc91e588c3db8f5c42322be
[fw-org]: https://github.com/lg-flutter-webos
[fw-8]: https://github.com/lg-flutter-webos/flutter-webos/issues/8
[fw-9]: https://github.com/lg-flutter-webos/flutter-webos/issues/9
[fw-10]: https://github.com/lg-flutter-webos/flutter-webos/issues/10
[fw-12]: https://github.com/lg-flutter-webos/flutter-webos/issues/12
[fwp]: https://github.com/lg-flutter-webos/plugins/tree/3a742bb86b04e261044f0864f9b1f71acf0bbad3
[ose-appinfo]: https://www.webosose.org/docs/guides/development/configuration-files/appinfo-json/
[ose-native]: https://www.webosose.org/docs/guides/development/native-apps/native-app-overview/
[ose-native-tut]: https://www.webosose.org/docs/tutorials/native-apps/developing-downloadable-native-apps/
[ose-sam-api]: https://www.webosose.org/docs/reference/ls2-api/com-webos-service-applicationmanager/
[ose-ls2]: https://www.webosose.org/docs/reference/luna-service2-library/
[ose-sec]: https://www.webosose.org/docs/guides/development/configuration-files/security-guide/
[ose-cmake]: https://github.com/webosose/cmake-modules-webos/blob/82f4f25baedfed2ba33c7ee707f898f18da44b19/webOS/webOS.cmake
[sam]: https://github.com/webosose/sam/tree/96161d3e50670fb21daccfb12a75c35fe406ce0e
[sam1]: https://github.com/webosose/sam/tree/28575218a7cdd970d80b5f789e94d72d77b2eb11
[ls2]: https://github.com/webosose/luna-service2/tree/5bc38aa348b35f5799f1bf9ed7c6cb513bc5f702
[wb]: https://github.com/webosbrew/webosbrew.github.io/tree/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides
[wb-devmode]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/devmode/index.md
[wb-toolbox]: https://github.com/webosbrew/dev-toolbox-cli/tree/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3
[wb-fwdata]: https://github.com/webosbrew/dev-toolbox-cli/tree/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/common/data
[wb-caniroot]: https://github.com/webosbrew/caniroot/tree/e32332def2b6ce08e42485aa14c933542692c22c
[wu]: https://github.com/webosbrew/webos-userland/tree/00a5f8721e15d5c10030b99b2f3e73e0c527bcce
[sdl]: https://github.com/webosbrew/SDL-webOS/tree/ba4c86af391d1524d3ff0cb61dc3b10035933239
[sdl3]: https://github.com/webosbrew/SDL-webOS/tree/f599b850f9b1347cebdbb9a806999ddd170ab661
[glasshouse]: https://github.com/rorygallagher2024/glasshouse/tree/567a17796de50052091f95524e02c455ce5b6b21
[nc4]: https://github.com/openlgtv/buildroot-nc4/tree/322ff04e625a7927338c2ac5f3d02cd0511c9b90
[nc4-glibc]: https://github.com/openlgtv/glibc/blob/fe819f0414e6385206dc8682c20d681ee0eb5998/version.h
[kodi]: https://github.com/xbmc/xbmc/tree/383db99b78d5c81453fa3fd5783334b17d9ac984
[gcc-abi]: https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html
