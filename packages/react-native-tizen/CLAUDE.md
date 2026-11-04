# react-native-tizen

React Native and Expo running natively on Samsung Tizen TVs: Hermes and Fabric
in `libreact-native-tizen.so`, drawn with EFL/Evas, loaded by a small .NET app
because the Samsung store accepts web and .NET apps only. Published as
`@kromatv/react-native-tizen` (MIT) from this repository; nothing here may
depend on KROMA or any other app.

## Read the decisions, then the reference

[`docs/decisions.md`](docs/decisions.md) records the choices the package is
built on (the .NET shell, `Platform.OS === 'tizen'`, tvOS-style remote events,
one logical 1920x1080 screen, self-registering modules and views). Follow them;
change one only by changing that file in the same change.

## Read the reference before writing platform code

[`docs/reference/`](docs/reference/README.md) holds what Tizen, EFL, React Native
and Expo actually require, with sources. Before changing code in an area, read its
file; when the code needs a fact that is not there, research it from the
official source and add it, cited, in the same change. Never implement a
platform behaviour (a key name, a manifest field, an Evas rule, an event payload)
from memory.

| Code | Reference |
|------|-----------|
| `shell/`, `src/package/`, `src/tizen/`, manifests, signing | `tizen-tv-platform.md` |
| `src/sdb/`, `src/tizen/devices.ts`, `src/tizen/install-app.ts` | `sdb-protocol.md` |
| `src/discovery/` | `tv-discovery.md` |
| `app.plugin.js`, `src/config/config-plugin.ts`, `src/commands/init*.ts`, `src/dependencies/`, `src/devtools/`, `devtools/`, `expo-module.config.json` | `expo-cli-integration.md` |
| `src/signing/`, `src/tizen/signing-identity.ts`, `src/tizen/profiles-file.ts`, `src/tizen/profile-password.ts` | `tizen-package-signing.md` |
| `runtime/host/src/input/` | `tizen-input.md` |
| `<TextInput>`: `views/TextInput*`, `fabric/TextInput*`, `input/KeyboardFocus`, `input/Ime*` | `text-input.md` |
| video, `expo-video` | `tizen-media-playback.md` |
| `runtime/host/src/views/`, `text/`, `mounting/` | `efl-rendering.md` |
| `src/libraries/`, `runtime/cmake/Libraries.cmake`, `runtime/host/src/svg/` | `../native-modules.md`, `efl-rendering.md` |
| `runtime/host/src/` (host, Fabric, TurboModules) | `react-native-platform-contract.md` |
| `runtime/host/devsupport/`, `src/package/dev-server-bundle.ts`, `--dev-server` | `react-native-dev-tooling.md` |
| `runtime/host/expo/` | `expo-modules-contract.md` |
| `runtime/host/src/tv/`, `shell/Voice*`, `shell/Preview*`, `shell/TilePayload.cs`, `src/api/`, `src/config/tv-features.ts` | `tizen-tv-app-integration.md`, `tizen-smart-hub-preview.md`, `tizen-tv-platform.md` section 7 |
| `runtime/cmake/Icu.cmake`, `runtime/intl/`, `runtime/host/src/intl/`, `src/runtime/icu-*`, `react-native-overrides/intl/` | `intl.md` |

## Verify on a TV, or in the simulator

In this repository `rn-tizen` is `bun src/cli/main.ts ... --project example`,
or `bun ../src/cli/main.ts` from `example/`.

- The simulator (`simulator/README.md`) builds the same runtime for aarch64 Linux.
  Use it for every UI change: from an app, `rn-tizen simulator run --no-logs`,
  then `key Down Enter`, `screenshot --out <file.png>` (and read the image),
  `logs -n 50`, and `crash` for a native backtrace. After a change in
  `runtime/`, `rn-tizen simulator build && rn-tizen simulator restart`.
- Other agents use the same VM. First pick an instance name no one else uses,
  such as your task's slug, and put it on every simulator command, as
  `--instance <name>` or `RN_TIZEN_SIMULATOR_INSTANCE=<name> rn-tizen simulator ...`
  (an `export` may not outlive one shell call). Every command then drives your
  own screen and app only. Never `stop` or `restart` another instance, never
  `delete` the VM, and expect `build` to wait while another agent builds the
  same app's libraries: apps with the same native libraries share a runtime
  build, so your `restart` runs the last one anyone built of it.
- The real TV is the final check: `rn-tizen run --device <ip>` from an app. Retail
  TVs block dlog, so the app streams its logs to `rn-tizen`.
- Agents share the TV. Hold `/tmp/rn-tizen-tv.lock` for everything that reaches
  it, remote keys (`rn-tizen key`, scripts on port 8002) as much as sdb: `mkdir`
  it, write who and since when to its `owner` file, `rm -rf` it when done, and
  wait while another agent holds it. A key sent without it lands in that agent's
  app.
- A behaviour seen on hardware goes into the reference with the TV's model,
  year and Tizen version.

## Layout

| Path | What |
|------|------|
| `runtime/` | the native runtime: CMake, the Tizen toolchain, the C++ host (`host/`) |
| `src/` | the `rn-tizen` CLI and the Metro integration (`src/metro/with-tizen.ts`) |
| `dist/` | the compiled JavaScript npm ships (`bun run build`, run by `prepare` on every install and before packing); never edited, never committed |
| `src/api/` | what apps import: `@kromatv/react-native-tizen/voice`, `/preview`, `/preview-json` (its own tsconfig: React Native's types, not Bun's) |
| `shell/` | the .NET app template every package is generated from |
| `react-native-overrides/` | the few React Native JavaScript files Tizen replaces |
| `simulator/` | the aarch64 Linux simulator |
| `example/` | an Expo app that runs on a TV |
| `docs/` | user docs (`cli.md`, `tv-features.md`, ...) and the platform reference |
