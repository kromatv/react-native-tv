# react-native-webos

React Native and Expo running natively on LG webOS TVs: Hermes, Fabric and the
host in one executable, `react-native-webos`, drawn with Skia on OpenGL ES in a
window SDL opens, packaged as a native `.ipk`. Published as
`@kromatv/react-native-webos` (MIT) from this repository; nothing here may
depend on KROMA or any other app. It is the webOS sibling of
[react-native-tizen](https://github.com/kromatv/react-native-tizen), ported
from it: where the two can share a design, they do.

## Read the decisions, then the reference

[`docs/decisions.md`](docs/decisions.md) records the choices the package is
built on (a native executable and no shell, Skia and SDL, `Platform.OS ===
'webos'`, tvOS-style remote events, one logical 1920x1080 screen,
self-registering modules and views). Follow them; change one only by changing
that file in the same change.

## Read the reference before writing platform code

[`docs/reference/`](docs/reference/README.md) holds what webOS, Skia, SDL, React
Native and Expo actually require, with sources. Before changing code in an
area, read its file; when the code needs a fact that is not there, research it
from the official source and add it, cited, in the same change. Never implement
a platform behaviour (a key code, an appinfo field, a Luna call, a Skia rule,
an event payload) from memory.

| Code | Reference |
|------|-----------|
| `src/package/`, `src/config/`, appinfo, lifecycle, paths, `runtime/host/src/platform/AppPaths*`, `LunaBus*` | `webos-tv-platform.md` |
| `src/package/` (the ipk writer) | `webos-ipk-format.md` |
| `src/webos/` | `webos-device-protocol.md` |
| `src/remote/` | `webos-remote-control.md` |
| `src/discovery/` | `tv-discovery.md` |
| `runtime/cmake/toolchain/`, `runtime/compat/`, `src/sysroot/`, what is linked from the TV and what is compiled in | `webos-native-toolchain.md` |
| `runtime/host/src/platform/HostWindow*`, `MainLoop*`, `runtime/host/src/input/`, `runtime/cmake/Sdl.cmake` | `webos-window-input.md` |
| `<TextInput>`: `views/TextInput*`, `fabric/TextInput*`, `input/KeyboardFocus`, `input/Ime*` | `text-input.md`, `webos-window-input.md` section 10 |
| `runtime/host/src/render/`, `views/`, `text/`, `mounting/`, `runtime/cmake/Skia.cmake`, `runtime/skia/` | `skia-rendering.md` |
| video, `expo-video` | `webos-media-playback.md` |
| `src/libraries/`, `runtime/cmake/Libraries.cmake`, `runtime/host/src/svg/` | `../native-modules.md`, `skia-rendering.md` |
| `runtime/host/src/` (host, Fabric, TurboModules) | `react-native-platform-contract.md` |
| `runtime/host/devsupport/`, `src/package/dev-server-bundle.ts`, `--dev-server` | `react-native-dev-tooling.md` |
| `runtime/host/expo/` | `expo-modules-contract.md` |
| `app.plugin.js`, `src/config/config-plugin.ts`, `src/commands/init*.ts`, `src/dependencies/`, `src/devtools/`, `devtools/`, `expo-module.config.json` | `expo-cli-integration.md` |
| `runtime/cmake/Icu.cmake`, `runtime/intl/`, `runtime/host/src/intl/`, `src/runtime/icu-*`, `react-native-overrides/intl/` | `intl.md` |

## Verify in the simulator, then on a TV

In this repository `rn-webos` is `bun src/cli/main.ts ... --project example`,
or `bun ../src/cli/main.ts` from `example/`.

- The simulator (`simulator/README.md`) builds the same runtime for aarch64
  Linux, with stock SDL on X11 and Mesa's OpenGL ES in place of the TV's
  compositor and GPU. Use it for every UI change: from an app, `rn-webos
  simulator run --no-logs`, then `key Down Enter`, `screenshot --out <file.png>`
  (and read the image), `logs -n 50`, and `crash` for a native backtrace. After
  a change in `runtime/`, `rn-webos simulator build && rn-webos simulator
  restart`.
- Other agents use the same VM. First pick an instance name no one else uses,
  such as your task's slug, and put it on every simulator command, as
  `--instance <name>` or `RN_WEBOS_SIMULATOR_INSTANCE=<name> rn-webos simulator ...`
  (an `export` may not outlive one shell call). Every command then drives your
  own screen and app only. Never `stop` or `restart` another instance, never
  `delete` the VM, and expect `build` to wait while another agent builds the
  same app's libraries: apps with the same native libraries share a runtime
  build, so your `restart` runs the last one anyone built of it.
- The real TV is the final check: `rn-webos run --device <name>` from an app,
  on a TV in Developer Mode. A retail TV shows an app no log, so the app streams
  its logs to `rn-webos`.
- A behaviour seen on hardware goes into the reference with the TV's model,
  year and webOS version. Nothing in the reference is verified on hardware
  until a row says so.

## Layout

| Path | What |
|------|------|
| `runtime/` | the native runtime: CMake, the webOS toolchain, Skia's build (`skia/`), the C++ host (`host/`) |
| `src/` | the `rn-webos` CLI and the Metro integration (`src/metro/with-webos.ts`) |
| `dist/` | the compiled JavaScript npm ships (`bun run build`, run by `prepare` on every install and before packing); never edited, never committed |
| `react-native-overrides/` | the few React Native JavaScript files webOS replaces |
| `simulator/` | the aarch64 Linux simulator |
| `example/` | an Expo app that runs on a TV |
| `docs/` | user docs (`cli.md`, ...) and the platform reference |
