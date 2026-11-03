# Design decisions

The choices react-native-tizen is built on, why, and what each one commits the
code to. Change one only by changing this file in the same change. Facts behind
them are in [`reference/`](reference/README.md).

## A .NET app that loads a native library

Samsung's store accepts web apps and .NET apps only. A small C# app (`shell/`)
starts, calls `rn_tizen_initialize()` before the app framework starts EFL, then
loads `libreact-native-tizen.so`, which holds Hermes, React Native's C++ core and
the host, and draws its own window. flutter-tizen ships the same way.

Consequence: publishing in the store needs a Seller Office partner group, since
Samsung reserves signing native libraries to partners
([tizen-tv-platform.md](reference/tizen-tv-platform.md)). Developer Mode works
with a Public certificate.

## LLVM, a static GCC runtime, the oldest rootstrap

The runtime is compiled with clang and linked with lld against a mirror of the
oldest supported Tizen rootstrap (9.0, glibc 2.30), with GCC 14's libstdc++
linked statically and three glibc symbols supplied by a hidden shim. One build
then runs on every newer TV. Samsung's own GCC for macOS needs Intel Homebrew
libraries, and lld rejects the rootstrap's stubs as shipped, hence the mirror
(`runtime/cmake/toolchain/tizen.cmake`, `src/sysroot/`).

## ICU inside the runtime, FormatJS for the rest of `Intl`

Every app gets `Intl` and Unicode-correct strings. Tizen 9 ships ICU 74 and
Tizen 10 ICU 76 (different sonames), and Hermes calls ICU's C++ API, which ICU
only promises across releases after a recompile, so the TV's ICU is not linked.
The runtime compiles ICU 78.3 from source into `libreact-native-tizen.so`, its
symbols hidden, and builds Hermes' Unicode layer and Intl on it: collation, case
mapping and normalization of every string, `localeCompare`, `Intl.Collator`,
`Intl.DateTimeFormat`, `Intl.getCanonicalLocales` and `toLocaleString` on dates.
Hermes' ICU backend has no `PluralRules` or `RelativeTimeFormat` on any platform,
a stub `NumberFormat`, and `toLocaleUpperCase` that returns the word "uppered":
the runtime replaces the case mapping with ICU's (`runtime/intl/CaseMapping.cpp`),
and every Tizen bundle carries FormatJS's `PluralRules`, `NumberFormat` (with
`Number.prototype.toLocaleString`) and `RelativeTimeFormat`, forced over
Hermes' own, defaulting to the locale Hermes resolves for the device
(`react-native-overrides/intl/`).

ICU's data is not linked. Each package ships `icudt78l.dat` in `res/`, trimmed to
the languages the config plugin's `locales` prop names (default `["en"]`): the main, collation,
currency and time-zone-name data of each language and its regional variants,
root, and the shared tables. The host points ICU at it before Hermes starts. A
device whose language the app does not list formats in the first listed
language, alphabetically.

| | `en` | `en`, `fr` | 12 languages |
|---|---|---|---|
| `icudt78l.dat` in `res/` | 2.49 MB | 2.58 MB | 4.61 MB |
| FormatJS in the bytecode bundle | +226 KB | +285 KB | +820 KB |

| Runtime | without ICU | with ICU |
|---|---|---|
| armv7 TV build, stripped | 9.02 MB | 11.34 MB (+1.05 MB gzipped) |
| aarch64 simulator build, stripped | 8.65 MB | 11.31 MB |

In the simulator, loading FormatJS with `en` and `fr` data takes 1.1 ms at
startup, and the first ICU collation and date format 2.0 ms (five starts each).
KROMA's French and English calls give Node 26.5's strings (ICU 78.3) but one:
English times keep ICU's U+202F before AM/PM where Node prints a space.

Rejected:

- **Hermes' ICU backend alone**: no `PluralRules` or `RelativeTimeFormat`, and
  its `NumberFormat` formats with `std::to_string`.
- **The TV's ICU**: one soname per year, C++ ABI not kept across releases.
- **FormatJS alone, Hermes without ICU**: FormatJS has no `Collator`; Hermes'
  lite tables compare by code unit, leave non-ASCII case and `normalize()`
  untouched and print "dateFormat not implemented"; FormatJS's `DateTimeFormat`
  needs 0.8 MB of time zone data to show local time.

Not covered: `formatToParts` of `DateTimeFormat` (Hermes returns one part),
`Intl.ListFormat`, `DisplayNames`, `Segmenter`, `Locale` and
`supportedValuesOf`; an app that needs them adds FormatJS's polyfill. Facts and
sources: [intl.md](reference/intl.md).

## `Platform.OS === 'tizen'`, with Android's sources underneath

A bundle for the `tizen` platform resolves every module as Android would, React
Native's Android sources being the closest to a TV, then takes a `.tizen.*` file
beside the one found. Metro is never asked for the `tizen` platform itself,
since Expo's resolver throws for a platform it does not know
(`expo-modules-autolinking/src/platforms/index.ts:57-73`). Apps can tell Tizen
apart, as they can Windows or macOS. The price is the code that compares
`Platform.OS` with `'android'` or `'ios'`: `'tizen'` matches neither. File by
file, Tizen takes one branch. Where the Android one matters, this package
overrides the file (`react-native-overrides/`: `Alert`, the ScrollView and Image
view configs); where the iOS one works once its module answers, the host serves
that module (`LinkingManager`, `AccessibilityManager`). `TextInput.js` and
`TextInputState.js` choose the whole component by `Platform.OS`, so the Babel
transformer reads `Platform.OS` as `'android'` in those two files alone
(`src/metro/tizen-android-branch.ts`) and Tizen draws Android's
`AndroidTextInput`. `Platform.select` reads
`tizen`, then `android`, then `native`, then `default`, at run time and when
Expo inlines it at build time (`withTizen` wraps the Babel transformer for
that). Third-party libraries may still need a `tizen` branch.

## Remote presses arrive as tvOS reports them

`useTVEventHandler` receives one event per press with `eventKeyAction: 1`, at
its release, and a key held 300 ms sends `long<Key>` with `0` when the hold
starts and `1` when it ends. Only the keys tvOS reports holds for (select, the
arrows, play/pause) hold that way; any other key held is one press. The TV's
repeats of a held key never reach the app. That is what app code written for
non-Android TVs, KROMA's among them, expects. While a `<TextInput>` has the
keyboard, keys go to it first, and a key it uses is never a remote event. Back goes to `BackHandler`
(`hardwareBackPress`) once, at its release; when nothing handles it the app
exits. While a `Modal` is mounted, Back goes to the last one mounted as
`onRequestClose` instead, and `BackHandler` hears nothing, as on Android.

## Fatal JavaScript errors

A development build shows a native red error screen with the stack, even before
React rendered anything, and Back closes the app. A release build logs the error
and exits, as React Native does on Android and iOS; a TV left on a black screen
is worse. Both paths a fatal error takes, the runtime's error handler before the
bundle ran and `ExceptionsManager` after, end there.

## Logs travel over the network

Retail TVs keep no readable system log. A development build streams every log
line, and native crash reports, to `rn-tizen` over TCP.

## One logical 1920x1080 screen

Apps lay out for 1920x1080, whatever the window the TV gives the app, and the
host scales to it. TV apps are designed for that size; a TV that hands the app a
smaller surface still shows the same layout.

Consequence: the surface's `pointScaleFactor` is the largest scale at which
1920x1080 fits the window (2/3 in 1280x720), `Dimensions` reports 1920x1080 with
that scale, and every view, text and image is drawn at it. A window that is not
16:9 leaves the extra strip black.

## Rendering with Evas objects

Every mounted view is an Evas smart object holding its background and its
children, moved in canvas coordinates. Text is an Evas textblock, measured with
the same engine that draws it. The window asks for the GPU, and font sizes are
pixels (`EVAS_FONT_DPI=72`).

## Expo on Tizen

`globalThis.expo` is installed from expo-modules-core's own C++, and Expo modules
for Tizen are written in C++ and registered with `RN_TIZEN_EXPO_MODULE`. The
global `fetch` is `expo/fetch`, as on Android and iOS, served by the host's
`ExpoFetchModule`. React Native's own `fetch` works too, since the host serves
`BlobModule` and `FileReaderModule`: without them whatwg-fetch reads every body
as an ArrayBuffer and decodes it one byte per character
(`whatwg-fetch/fetch.js:202-210`), so UTF-8 text arrives as Latin-1.

## Everything registers itself

TurboModules (`RN_TIZEN_PLATFORM_MODULE`), native views
(`RN_TIZEN_COMPONENT_VIEW`), Fabric component descriptors
(`RN_TIZEN_COMPONENT_DESCRIPTOR`) and Expo modules (`RN_TIZEN_EXPO_MODULE`) add
themselves from their own file. A feature is new files, not an edit to a
central list.

## Native libraries come from codegen

A package with a `codegenConfig` is compiled into the runtime: its codegen C++
output, plus a Tizen implementation the package declares (`"tizen": { "cmake":
... }` in its package.json) or, failing that, one this package bundles under
`runtime/host/src/<dir>` with a `library.json`. A library with neither still lays
out. Each compiles as an OBJECT library linked into `libreact-native-tizen.so`,
so its registrations survive the link. See [native-modules.md](native-modules.md).

## TV services: the shell talks to Samsung, the host decides

Samsung's voice service and Smart Hub preview are reached through .NET APIs
and message ports, so the shell (`shell/VoiceChannel.cs`,
`shell/PreviewPublisher.cs`) speaks to them and nothing else: every decision
(which player obeys "pause", which view "select Dune" presses, where a search
goes) is the host's (`runtime/host/src/tv/`), reached through the C API, so the
simulator's launcher can stand in for the shell (`rn-tizen simulator voice`,
`tile`). An app turns each one on in the config plugin's props and imports it from
`@kromatv/react-native-tizen/voice` or `/preview`: TurboModules and
`DeviceEventEmitter` from `react-native`, so they need neither Expo nor a
platform check, and answer "not available" on Android, iOS and the web.

Consequence: these features rest on unpublished Samsung protocols and
assemblies (facts in [tizen-tv-app-integration.md](reference/tizen-tv-app-integration.md)
and [tizen-smart-hub-preview.md](reference/tizen-smart-hub-preview.md)), and a
store build must clear them with Samsung's content manager first; a release
`rn-tizen package` says so.


## The CLI reaches the TV itself

`rn-tizen` speaks sdb's wire protocol to the TV's daemon on port 26101 (`src/sdb/`):
the handshake, `capability:`, a `sync:` push and the TV's `0 vd_appinstall` and
`0 execute` commands, as Tizen Studio's `sdb` sends them on the same TV
([sdb-protocol.md](reference/sdb-protocol.md)). Tizen Studio's `sdb` is never
run, and with no sdb server on the machine, `rn-tizen` remembers the TVs it
reached in its state directory.

Consequence: each command opens its own connection and closes it, because the
TV takes one at a time; while Tizen Studio is connected to a TV, `rn-tizen`
cannot reach it, and says so. Only the commands seen on hardware are sent, so
another model year that answers differently fails with the TV's own words
rather than a guess at a fallback.

## Packages are signed in TypeScript, with Samsung's certificates

`rn-tizen` signs the `.tpk` itself (`src/signing/`): the author signature and the
distributor signature `tizen package -t tpk` writes, byte for byte, from the
certificates of a Tizen Studio profile or of `.p12` files given on the command
line. PKCS#12 is read in TypeScript, RC2 included, because Node's OpenSSL 3 has
none and Samsung encrypts its certificates with it.

Consequence: the one part of Tizen Studio a TV still needs is Samsung's
Certificate Manager, used once, since a retail TV takes an author and a
distributor only from Samsung's chains: a self-signed author is refused on
hardware ([tizen-package-signing.md](reference/tizen-package-signing.md)
section 5). Where Tizen Studio and Tizen's validator disagree, a file named like
a signature below the package root, the package follows the validator: the TV
refused Tizen Studio's package and installed this one.

## Everything Tizen has a Node API

What `rn-tizen` does to a TV is exported from `@kromatv/react-native-tizen/tizen`
(`src/tizen/index.ts`): the sdb client, discovery, signing, PKCS#12 and the
profiles, with their types. Another tool, such as a deployer for a web app's
`.wgt`, imports it instead of carrying its own copy, and the CLI commands are
thin layers over the same functions.

Consequence: that entry point is public API under semver, documented in
[tizen-api.md](tizen-api.md); a breaking change to an export is a breaking
release.

## Expo sees a config plugin and a dev tools plugin

The Tizen settings are the props of this package's config plugin
(`app.plugin.js`), so `npx expo install` adds it to `app.json` by itself and
`expo config` checks it. The package is also a dev tools plugin
(`expo-module.config.json`): `npx expo start` lists Run on TV, Run in simulator,
Find TVs and Create certificate under shift+m, and serves a page of their
progress and logs. A development build those commands make loads its bundle
from that dev server. Facts in [expo-cli-integration.md](reference/expo-cli-integration.md).

Consequence: Expo kills a command after ten seconds, so each one starts a
detached job and returns at once. Expo has no `run:tizen`, no Tizen key and no
Tizen in `prebuild` or `export`, so `rn-tizen` keeps its own commands, and an
app adds a `"tizen": "rn-tizen run"` script beside Expo's. `expo.tizen` is read
for one more release, with a deprecation.

## The package ships JavaScript, and the CLI runs on Node

Node refuses to strip types from a file under `node_modules`, and everything a
consumer's machine loads from this package goes through Node: Expo's Metro
config loader, Metro's transform workers and the `rn-tizen` bin. So
`bun run build` compiles `src/` to `dist/` with `tsc`, one file per source at
the same depth, which keeps every path taken from `import.meta.url` (the
package root, `react-native-overrides/`, `runtime/`, `shell/`, `simulator/`)
right in both trees. `exports` and `bin` point at `dist/`, and `prepack` builds
it, so neither `npm pack` nor `npm publish` can ship a stale or missing build.
`react-native-overrides/` and the app API under `src/api/` reach the app
through Metro, which compiles TypeScript under `node_modules` with the app's
Babel preset; the overrides ship as TypeScript, the API compiled with its
declarations.

The CLI uses Node's APIs only, so its `bin` runs on Node 22.13 or newer, the
floor Expo 58 and React Native 0.88 already set, with no Bun on the user's
machine; Bun runs it too. Contributors run the sources (`bun src/cli/main.ts`)
and the example loads `src/` by relative path, so nothing is built during
development.

Consequence: no source may use a Bun API, and a file that finds another
through `import.meta.url` must keep the same depth under `src/` and `dist/`.
CI builds `dist/` and runs the compiled CLI with Node.
