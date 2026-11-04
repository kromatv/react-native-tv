# Tizen versions

Which Samsung TVs a react-native-tizen app reaches, why the floor is Tizen 6.0,
what each version below it would cost, and how the same project could reach
the older TVs as a web app. The facts, with sources, are in
[`reference/tizen-tv-platform.md`](reference/tizen-tv-platform.md), section 2;
the decision is in [`decisions.md`](decisions.md).

## Which TVs a package reaches

| Model year | Tizen | .NET on the TV | This package |
|---|---|---|---|
| 2026 | 10.0 | .NET 8 | the `.tpk` |
| 2025 | 9.0 | .NET 6 | the `.tpk`, verified on hardware |
| 2024 | 8.0 | .NET 6 | the `.tpk` |
| 2023 | 7.0 | .NET 6 | the `.tpk` |
| 2022 | 6.5 | .NET Core 3.1 | the `.tpk` |
| 2021 | 6.0 | .NET Core 3.1 | the `.tpk`, the floor |
| 2020 | 5.5 | .NET Core 3.0 | a web build (designed below, not built) |
| 2018 to 2019 | 4.0, 5.0 | .NET Core 2.x | a web build |
| 2015 to 2017 | 2.3 to 3.0 | none | a web build, if the web engine allows (below) |

TVs from 2023 take OS upgrades, so a model year names the Tizen it shipped
with, not the one it runs.

## Why 6.0

1. **Samsung blocks an app's native libraries below 6.0.** flutter-tizen, which
   ships the same way (a .NET app loading its own `.so`), says "Flutter apps
   cannot be run on 2020 or earlier TV models (Tizen 5.5 or earlier) due to
   Samsung TV division's security policies"; those TVs answer "refers a
   forbidden assembly directly" or "A malicious process has been blocked".
   Nothing on the build side moves this wall.
2. **Every symbol the runtime imports exists from 6.0.** The 581 symbols a
   Tizen 9.0 build imports all resolve in the 6.0, 6.5, 7.0, 8.0, 9.0 and 10.0
   rootstraps; 5.5 lacks 13, 5.0 lacks 17, 4.0 lacks 19.
3. **The .NET shell compiles unchanged at TizenFX API 8** (`tizen80`, Tizen 6.0,
   .NET Core 3.1 reference assemblies). It needs nothing newer.
4. **Building against the 6.0 rootstrap takes two changes.** curl 7.68 declares
   `curl_url_get` with a non-const handle, and 6.0's headers comment out nine
   Evas VG functions that its libevas exports and 6.5 declares unchanged; the
   SVG views declare them when the headers do not
   (`runtime/host/src/svg/EvasVgLegacy.h`). The built runtime's 584 imports
   resolve on 6.0, 9.0 and 10.0.

The risk in point 4: on 6.0 the SVG views call functions Tizen 6.0 did not
publish. They are exported, so they load and link; whether Samsung's review
objects to them is a question for the Seller Office content manager. If it
does, the floor for an app using react-native-svg moves to 6.5, or the SVG
views draw nothing below 6.5.

## What each step lower costs

| Floor | Year | Cost |
|---|---|---|
| 6.0 | 2021 | done: the two build changes above |
| 5.5 | 2020 | Samsung's block on app `.so` files (no code fixes it). Then: glibc 2.24, so the shim grows by `getrandom`, `getentropy` and `pthread_cond_clockwait`; curl 7.62 lacks `curl_multi_poll` and `curl_multi_wakeup`, which the networking loop uses; EFL 1.23.99 instead of 1.25. The shell builds at `tizen60` unchanged |
| 5.0 | 2019 | 5.5's, and curl's URL API (`curl_url*`, 7.62) for WebSocket URLs; `tizen50` has no `System.Text.Json` (four shell files) and no `EventManager` (voice) on .NET Core 2.1 |
| 4.0 | 2018 | 5.0's, and EFL 1.16: no `evas_object_vg_root_node_set` (SVG), no `player_set_video_roi_area` (video in a view), Ecore Wayland 1 only |
| 3.0 and older | 2015 to 2017 | no .NET at all: only a web app runs |

## How a package targets the floor

- **`platform` is a floor.** The plugin's `platform` prop (or `--platform`)
  defaults to `6.0` and takes `6.0` to `10.0`. One `.tpk` per CPU reaches that
  version and every later one. A floor or a TV below 6.0 is refused, pointing
  here.
- **The runtime** is linked against the floor's rootstrap: IoT Headed for 6.0
  to 7.0, since Tizen Studio publishes no TV rootstrap and IoT Headed is the
  profile with armv7 and aarch64 ones; the unified `tizen` profile from 8.0.
  The x86_64 build, for the emulator, uses 10.0's: the only x86_64 TV
  emulator is 10.0, and the 8.0 and 9.0 x86_64 rootstraps carry IA-64's headers.
- **The shell** is built against TizenFX API 8 (`tizen80`) whatever the floor;
  the manifest's `api-version` is the floor, which is what the TV checks at
  install ("Failure occurs in step: CheckTizenVersion" when it is newer than
  the TV).
- **At run time** the host reads the TV's version from `system_info`
  (`http://tizen.org/feature/platform.version`). `Platform.Version` is that
  version as a number (`6.5`), `Platform.constants.Release` the string. Voice
  (Tizen 6.0) and title selection by voice (6.5) check it, and
  `Voice.unavailableReason` and `SmartHubPreview.unavailableReason` say why a
  feature is off ([tv-features.md](tv-features.md)).
- **Privileges** need no check: every privilege the shell and its features
  declare is Public in the .NET privilege list of every version from 4.0, and
  Samsung's `voicecontrol` exists from 6.0, the floor.

## What was verified

| Where | What |
|---|---|
| Build machine | At the 6.0 floor the runtime builds for armv7 (IoT Headed 6.0 `device`), aarch64 (IoT Headed 6.0 `device64`) and x86_64 (10.0 `emulator64`); every import resolves on the rootstraps of 6.0 to 10.0 for its CPU. A `6.0` package builds (`tizen80`, `api-version="6.0"`). |
| GQ75LS03DAUXZG, 2024 Frame, Tizen 9.0 (2026-11-04) | A 6.0-floor development package (runtime linked against IoT Headed 6.0, shell `tizen80`, `api-version="6.0"`) installs, launches and runs the example's platform screens: voice registers with Samsung's service, `Platform.Version` is 9, `Voice.unavailableReason` null, `canSelectTitles` true, a video plays in a view, and a Smart Hub preview publishes through `ContentDBAPI` |
| TV emulators | Not run, for three reasons below |

The emulators cannot stand in here:

- Samsung's repository has TV emulator images for 7.0, 8.0, 9.0 and 10.0 only
  ([tv_extensions snapshots][tv-ext]); there is no 6.0 or 6.5 image.
- The 7.0 to 9.0 images are 32-bit x86. Tizen Studio's only i586 GCC is 9.2,
  whose libstdc++ lacks the C++20 headers React Native 0.88 includes, so the
  runtime cannot be built for them.
- On an Apple Silicon Mac the emulator is x86 QEMU translated twice (TCG under
  Rosetta). The 10.0 x86_64 image's userland faults (`launchpad-process-pool`
  segfaults); the 9.0 image reaches `launchpad` with no faults but its sdb port
  never answers. An Intel or AMD machine with hardware virtualization is
  needed, and a Samsung distributor certificate that lists the emulator, since
  TV Extension 7.0.1 installs only Samsung-signed apps.

## A web build for older TVs

Not built. How `rn-tizen` could reach 2015 to 2020 TVs from the same project.

### What Samsung's store allows

- One Seller Office application has one App Type, chosen when it is created
  and not updatable: "a Tizen web application or a .NET application". A
  multi-package upload "must have only wgt files or tpk files"
  ([so-register]).
- "Two different Tizen applications cannot be registered with the TV Seller
  Office using the same Tizen ID", and an update keeps its Tizen ID
  ([so-register]).
- One application covers several model groups, and each release picks its
  groups: "Submit an application update for each application version, making
  sure you select the applicable model groups for each one." To manage an app
  "for specific model groups under a separate application name", file a
  "1:1 Q&A" ticket for the App Management team ([so-faq]).
- "For a Tizen .NET application, the service can be released for TV model
  groups for 2018 and later", and "the default policy is not to operate
  different application versions for each Tizen version": exceptions go
  through the content manager ([so-distribute]).

So the `.tpk` and the `.wgt` are two Seller Office applications with two Tizen
IDs, each released to its own model groups, and Samsung has to agree to the
pair.

### Routing by model group

| Seller Office application | Package | Model groups ([sam-mg]) |
|---|---|---|
| the app (.NET) | `.tpk`, floor 6.0, Tizen ID `org.example.app` | 2021 and later |
| the app for older TVs (web) | `.wgt`, `required_version` 2.3, Tizen ID `AbCdEfGhIj.App` | 2015 to 2020 |

The install gate backs the routing up: a `.tpk` with `api-version` 6.0 does
not install on a 5.5 TV ([sam-baseline]), and a web app whose
`required_version` is 2.3 "can run on both 2016 and 2015 TVs" ([so-launch]).
Releasing the `.tpk` only to 2021 groups matters even though Samsung allows
.NET from 2018: those TVs would install it and then block its runtime.

### Building both from one project

```sh
rn-tizen package            # the .tpk, as today
rn-tizen package --web      # the .wgt
```

1. **Bundle.** `expo export --platform web` with react-native-web, Babel
   switched to the old engines' targets. `babel-preset-expo`'s web preset adds
   no `preset-env` ([expo-web-preset]), so `withTizen` would add one, with
   core-js, when `RN_TIZEN_WEB` is set.
2. **Engine floor.** Samsung lists Chromium 56 for 2018, 63 for 2019, 69 for
   2020, Chromium 47 for 2017 and WebKit for 2016 ([sam-web]).
   react-native-web is "designed and tested for recent mobile and desktop
   browsers", Chrome 60 and later, and asks for `Promise`, `Object.assign`,
   `Array.from` and `ResizeObserver` polyfills ([rnw-compat]). 2018 to 2020 is a
   transpile and polyfill job. 2015 to 2017 lacks arrow functions (2.3, 2.4) or
   classes, `Promise`, `Map` and `Symbol` (3.0) natively ([sam-web]), which
   React assumes ([react-18]): reaching them means ES5 and a full core-js, and a
   test on a 2016 set before promising it.
3. **Remote.** A small web entry from this package
   (`@kromatv/react-native-tizen/web`) registers the media and colour keys with
   `tizen.tvinputdevice.registerKey` and turns key events into the same
   tvOS-style events and `hardwareBackPress` the native host sends
   ([decisions.md](decisions.md), "Remote presses arrive as tvOS reports them").
4. **Package.** A generated `config.xml`: the web Tizen ID, `required_version`
   2.3, profile `tv-samsung`, the `internet` privilege, `content src="index.html"`.
   Signed with the same profiles and code as the `.tpk` (`src/signing/`), as
   KROMA's web app already is.
5. **TV features.** Voice and the personal preview stay off on the web build,
   their `unavailableReason` saying so: VoiceInteraction needs 6.0, and the web
   preview is a separate service app.

### Questions for Samsung's content manager

- Will Samsung pair a web application for 2015 to 2020 groups with a .NET one
  for 2021 and later, and under the same name?
- Does the review accept functions Tizen 6.0 exports but did not publish
  (the nine Evas VG functions above)?
- The partner-group rule for native `.so` files ([sam-netfaq] Q3) applies to
  the `.tpk` whatever its floor.

[tv-ext]: https://download.tizen.org/sdk/extensions/tv_extensions/snapshots/
[so-register]: https://developer.samsung.com/tv-seller-office/guides/applications/registering-application.html
[so-faq]: https://developer.samsung.com/tv-seller-office/faq/application-registration.html
[so-distribute]: https://developer.samsung.com/tv-seller-office/guides/applications/distributing-application.html
[so-launch]: https://developer.samsung.com/tv-seller-office/checklists-for-distribution/launch-checklist.html
[sam-mg]: https://developer.samsung.com/smarttv/develop/specifications/tv-model-groups.html
[sam-baseline]: https://developer.samsung.com/tizen/Smart-TV/FAQ/Tizen-baseline-SDK.html
[sam-web]: https://developer.samsung.com/smarttv/develop/specifications/web-engine-specifications.html
[sam-netfaq]: https://developer.samsung.com/smarttv/develop/faq/tizen-net-tv.html
[rnw-compat]: https://necolas.github.io/react-native-web/docs/browser-compatibility/
[expo-web-preset]: https://github.com/expo/expo/blob/main/packages/babel-preset-expo/src/configs/web.ts
[react-18]: https://react.dev/blog/2022/03/29/react-v18
