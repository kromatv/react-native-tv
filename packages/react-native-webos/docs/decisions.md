# Design decisions

The choices react-native-webos is built on, why, and what each one commits the
code to. Change one only by changing this file in the same change. Facts behind
them are in [`reference/`](reference/README.md). The package is a port of
react-native-tizen: a decision that reads the same there is the same decision.

## A native executable in an `.ipk`, and no shell

A webOS native app is an ELF the app manager starts in a jail, named by `main`
in `appinfo.json`. The package's one executable, `react-native-webos`, holds
Hermes, React Native's C++ core, the host, Skia, SDL and ICU, and opens its own
window. Nothing loads it: where Samsung's store forced react-native-tizen behind
a .NET app, webOS needs no shell, so the host itself parses the launch
parameters, registers with the app manager and talks to the TV's services.

Consequence: who may publish a native app in LG's store is LG's decision, and
its public pages name web apps only; LG's own Flutter tool produces native
packages for webOS 26. Until LG says otherwise, a package runs on a TV in
Developer Mode and is distributed as homebrew is
([webos-tv-platform.md](reference/webos-tv-platform.md), section 1). No
document here may promise LG Content Store.

## Skia draws, SDL opens the window

webOS has no UI toolkit a native app may link: it gives an app a Wayland
surface and OpenGL ES. The host draws every view with Skia (Ganesh on OpenGL
ES 2 or 3, milestone m157, built from pinned sources by Skia's own GN build)
and lays out text with SkParagraph, so the engine that measures a paragraph is
the one that draws it. The window, the OpenGL context, remote keys, the
pointer and the on-screen keyboard come from SDL 3, webosbrew's fork, linked
statically: the TV's own SDL is a different library on each webOS release, and
webOS moves its Wayland protocol requests between releases, which the fork
already handles ([webos-window-input.md](reference/webos-window-input.md),
sections 4 to 6; [skia-rendering.md](reference/skia-rendering.md)).

Consequence: Skia is built once per machine and target, outside any one
runtime build (`RN_WEBOS_SKIA_ROOT`), because it takes minutes and does not
depend on the app. The simulator runs the same Skia and the same SDL source,
on X11 and Mesa. Skia's API moves between milestones: a name is looked up in
the pinned source, never remembered.

## One scene, redrawn when it changes

A mounted view is plain state: its box, its transform, what it shows. Nothing
is drawn until a view changes; the next display frame then walks the views and
draws the whole scene, back to front, culling what the clip rejects
(`render/Scene`, `views/ComponentView`). A view's opacity goes into its paint
when it draws one thing, and into a layer only when it draws several. There is
no retained object graph to keep in step, which is what most of
react-native-tizen's view code does for Evas.

Consequence: a frame's cost is the visible views, so a view must not allocate,
decode or lay out while drawing: images decode off the main thread, a
paragraph is laid out once and kept. Repainting only what changed waits for a
measurement on a TV ([skia-rendering.md](reference/skia-rendering.md),
section 7).

## The host owns the main loop

One loop on the main thread waits on SDL's events, runs tasks posted from
other threads, timers, and one tick per display frame while something asks for
frames (`platform/MainLoop`). The main thread alone touches the window, the
scene and the views. JavaScript runs on its own thread; Fabric lays out there
and measures text there, under the text engine's lock.

## LLVM, a static GCC runtime, one sysroot older than any TV

The runtime is compiled with clang and linked with lld against openlgtv's SDK:
a glibc 2.12.2 sysroot, older than the oldest TV's 2.14.1, with GCC 16.2's
libstdc++, libgcc and libatomic linked statically. One 32-bit armv7 executable
(softfp, NEON, Thumb-2) then runs on every release; no webOS release gives an
app a 64-bit userland. What the sources call beyond glibc 2.12.2 is defined,
hidden, in `runtime/compat/`. From the TV the executable links only libraries
every release has under the same name, and the build fails on any other
(`runtime/cmake/WebOSPlatform.cmake`); everything else is compiled in or opened
with `dlopen`. It is linked at a fixed address, as the SDK's own executables
are: the SDK's `libatomic.a` is not position independent. The SDK is built for
a Linux host and serves macOS too, since clang reads only its target files
([webos-native-toolchain.md](reference/webos-native-toolchain.md)).

Consequence: no TV was at hand for the port, so `scripts/check-tv-binary.ts`
checks every executable against webosbrew's firmware symbol data, 14 images
from webOS 1.2 to 11.2, and that is the evidence until a TV runs one.

## webOS 4 is the floor

One executable runs on webOS TV 4.x (2018) and every later release. Every
image back to 1.2 has what the executable imports, and SDL finds every Wayland
function it requires from 3.4 on; 4 is the floor because the jailer mis-sets an
app's directory permissions before it and native apps are only properly
supported from it ([webos-native-toolchain.md](reference/webos-native-toolchain.md),
sections 4 and 9). A feature that needs a newer webOS checks the TV's version
at run time and says why it is off.

## libcurl and Mbed TLS inside the runtime

A TV's libcurl changes SONAME at webOS 5 and lacks calls the host makes before
webOS 7; its OpenSSL has four SONAMEs, no TLS 1.3 before webOS 6 and a root
list as old as the firmware. The runtime compiles libcurl with Mbed TLS in,
HTTP and HTTPS only, and every package ships Mozilla's roots as
`res/cacert.pem`, which the host names for every transfer. The simulator links
the same two.

Consequence: a TLS fix reaches users with a release of this package, not with
the TV's firmware.

## ICU inside the runtime, FormatJS for the rest of `Intl`

As in react-native-tizen: the runtime compiles ICU from source, statically,
symbols hidden, and each package ships an `icudt<version>l.dat` trimmed to the
app's locales; Hermes' Intl and Unicode functions run on it, and every bundle
carries FormatJS's `PluralRules`, `NumberFormat` and `RelativeTimeFormat` over
Hermes' gaps ([intl.md](reference/intl.md)). On webOS the same ICU also breaks
text into lines for Skia, so the data keeps ICU's break rules and the
dictionaries of the app's scripts.

## `Platform.OS === 'webos'`, with Android's sources underneath

A bundle for the `webos` platform resolves every module as Android would,
React Native's Android sources being the closest to a TV, then takes a
`.webos.*` file beside the one found. Metro is never asked for the `webos`
platform itself, since Expo's resolver throws for a platform it does not know.
Apps can tell webOS apart, as they can Windows or macOS. The price is the code
that compares `Platform.OS` with `'android'` or `'ios'`: `'webos'` matches
neither. Where the Android branch matters, this package overrides the file
(`react-native-overrides/`); `Platform.select` reads `webos`, then `android`,
then `native`, then `default`.

## Remote presses arrive as tvOS reports them

`useTVEventHandler` receives one event per press with `eventKeyAction: 1`, at
its release, and a key held 300 ms sends `long<Key>` with `0` when the hold
starts and `1` when it ends. Only the keys tvOS reports holds for hold that
way. The TV's repeats of a held key never reach the app. While a `<TextInput>`
has the keyboard, keys go to it first. Back goes to `BackHandler`
(`hardwareBackPress`) once, at its release, or to the last `Modal` mounted as
`onRequestClose`.

Consequence: Back reaches an app only because its surface claims it; Exit and
Home are never claimed, so the TV can always close the app. Keys are matched
on SDL's scancodes, and on the Linux key code where SDL has none
([webos-window-input.md](reference/webos-window-input.md), section 15).

## The pointer is a mouse

LG's remote has a pointer, and an app that ignores it feels broken on an LG
TV. It reaches the app as a mouse: moving it sends W3C pointer enter and leave
events, which `Pressable` reports as `onHoverIn` and `onHoverOut`; its button
sends touches, which React Native's responder system turns into presses; its
wheel scrolls the nearest scroll view. react-native-tizen has no counterpart.

Consequence: there is still no native focus engine: `focusable`,
`hasTVPreferredFocus` and `TVFocusGuideView` do nothing, and an app keeps its
focused item in state. An app that wants the pointer to move that focus does
it in `onHoverIn`.

## Fatal JavaScript errors

A development build shows a native red error screen with the stack, even
before React rendered anything, and Back closes the app. A release build logs
the error and exits.

## A development runtime and a release runtime

The executable comes in two flavours. A development build carries
`runtime/host/devsupport/`, which talks to a dev server when the runtime
config has a `bundleUrl`, and Hermes built with its debugger. A release build
is compiled without any of it. CMake's `RN_WEBOS_DEVELOPMENT` picks the
flavour, and each has its own build directory.

## Logs travel over the network

A retail TV shows a developer no log of a native app. A development build
streams every log line, and native crash reports, to `rn-webos` over TCP.

## One logical 1920x1080 screen

Apps lay out for 1920x1080, whatever surface the TV gives the app, and the
host scales to it. A UHD TV gives an app a 1920x1080 surface; a Full HD model
gives 1280x720, where the point scale is 2/3.

## Expo on webOS

`globalThis.expo` is installed from expo-modules-core's own C++, and Expo
modules for webOS are written in C++ and registered with
`RN_WEBOS_EXPO_MODULE`. The global `fetch` is `expo/fetch`, served by the
host's `ExpoFetchModule`; React Native's own `fetch` works too.

## Everything registers itself

TurboModules (`RN_WEBOS_PLATFORM_MODULE`), native views
(`RN_WEBOS_COMPONENT_VIEW`), Fabric component descriptors
(`RN_WEBOS_COMPONENT_DESCRIPTOR`) and Expo modules (`RN_WEBOS_EXPO_MODULE`) add
themselves from their own file. A feature is new files, not an edit to a
central list.

## Native libraries come from codegen

A package with a `codegenConfig` is compiled into the runtime: its codegen C++
output, plus a webOS implementation the package declares (`"webos": { "cmake":
... }` in its package.json) or, failing that, one this package bundles under
`runtime/host/src/<dir>` with a `library.json`. See
[native-modules.md](native-modules.md).

## The TV's services are Luna calls, made by the host

The app manager, the settings, the media pipeline and the rest of webOS answer
JSON calls on the Luna bus. The host reaches it through LG's `libhelpers.so.2`,
which every release has, opened at run time (`platform/LunaBus`), so nothing
links a library whose C++ interface changes between releases. Samsung's voice
service and Smart Hub preview, which react-native-tizen wraps, have no
counterpart here.

Consequence: in the simulator the bus is absent, and whatever depends on it
answers as on a TV that refuses the call.

## The CLI reaches the TV itself

`rn-webos` talks to a TV in Developer Mode over SSH, with the key the TV's own
key server hands out, and runs the Luna commands an install, a launch and a
removal take; it presses remote keys over the TV's second-screen protocol. LG's
`ares-*` commands are never run
([webos-device-protocol.md](reference/webos-device-protocol.md),
[webos-remote-control.md](reference/webos-remote-control.md)).

## Packages are written in TypeScript, and not signed

`rn-webos` writes the `.ipk` itself: an `ar` archive of a control file and a
data tree, as LG's CLI lays it out
([webos-ipk-format.md](reference/webos-ipk-format.md)). A webOS package carries
no signature, so the certificates, profiles and passwords of react-native-tizen
have no counterpart.

## Everything webOS has a Node API

What `rn-webos` does to a TV is exported from
`@kromatv/react-native-webos/webos`, and the CLI commands are thin layers over
the same functions. That entry point is public API under semver.

## Expo sees a config plugin and a dev tools plugin

The webOS settings are the props of this package's config plugin
(`app.plugin.js`), and the package is also a dev tools plugin
(`expo-module.config.json`): `npx expo start` lists Run on TV, Run in
simulator and Find TVs. Expo has no `run:webos`, so `rn-webos` keeps its own
commands. A config plugin cannot add a platform to Metro, so `withWebOS` wraps
the app's Metro config; `npx rn-webos init` writes it.

## The package ships JavaScript, and the CLI runs on Node

`bun run build` compiles `src/` to `dist/` with `tsc`, one file per source at
the same depth; `exports` and `bin` point at `dist/`, and `prepare` builds it.
The CLI uses Node's APIs only. No source may use a Bun API, and a file that
finds another through `import.meta.url` must keep the same depth under `src/`
and `dist/`.

## What a TV has verified is written down

The port was made without an LG TV: every platform fact comes from
documentation or source, and every behaviour was first checked in the
simulator only. Each reference file lists what a TV must settle, and a claim
becomes "verified on hardware" only with the TV's model, year and webOS version
beside it. [reference/hardware-verification.md](reference/hardware-verification.md)
is the record of what ran on which TV; the first run was on 2026-11-11, in
LG's Cloud Test Lab.
