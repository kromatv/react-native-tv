# react-native-tv

One setup for an Expo app on every TV: one entry in `app.json` that also sets
Metro up, `withTV` for an app without it, `run:<platform>` commands with a patch that gives Expo CLI those
commands, and one `t` key in `expo start`. Published as
`@kromatv/react-native-tv` (MIT) from this repository. It holds no platform
code: Samsung Tizen is
[react-native-tizen](https://github.com/kromatv/react-native-tv/tree/main/packages/react-native-tizen), LG webOS is
[react-native-webos](https://github.com/kromatv/react-native-tv/tree/main/packages/react-native-webos), and Apple
TV and Android TV are react-native-tvos. Its `package.json` brings those
packages to an app, and its code names none of them: it finds a platform by
what the platform's `package.json` declares. Nothing here may depend on KROMA
or on any other app.

## Read the decisions, then the reference

[`docs/decisions.md`](docs/decisions.md) records the choices the package is
built on (one dependency for an app and no platform in the code, a platform's `run` in its own process,
Expo CLI patched through the app's package manager, one `app.json` entry that
stands for each platform's own, one owner for the keys of `expo start`). Follow them; change one
only by changing that file in the same change.

[`docs/reference/`](docs/reference/README.md) holds what Expo CLI and the
package managers actually do, with sources. Before changing code in an area,
read its file; when the code needs a fact that is not there, read it in the
source or observe it, and add it, cited, in the same change.

| Code | Reference |
|------|-----------|
| `src/expo-cli/`, `src/expo-patch/` | `expo-cli.md` |
| `src/config/`, `app.plugin.js`, `src/start/`, `expo-module.config.json` | `expo-config-and-start.md`, `../platform-packages.md` |
| `src/platforms/`, `src/metro/` | `../platform-packages.md`, and `expo-cli-integration.md` in react-native-webos for how Expo loads a Metro config |

## Verify in an app

Unit tests do not prove a patch of Expo CLI, a config plugin or a key in
`expo start`. A change in `src/expo-cli/`, `src/expo-patch/`, `src/config/` or
`src/start/` is checked in a real app: pack this package
(`bun pm pack`), install the tarball beside `expo` and the platform packages,
run `rn-tv expo-patch`, then `npx expo run:<platform> --help` and
`npx expo --help`, and reinstall from scratch once. For the plugin, read
`expo config --json`; for the keys, drive `npx expo start` in a terminal and
keep reading its output while you wait, or the dev server blocks. The pull
request says which versions of `expo`, `@expo/cli` and the package manager ran
it, and which TV or simulator.

## Layout

| Path | What |
|------|------|
| `src/platforms/` | finding the TV platform packages among an app's dependencies |
| `src/metro/` | `withTV`, what apps import from `@kromatv/react-native-tv/metro`, and the Metro config the config plugin hands Expo |
| `src/expo-cli/` | the commands Expo CLI gains, loaded by the patch |
| `src/expo-patch/` | the patch of `@expo/cli` and the package manager that applies it |
| `src/config/` | the config plugin `app.plugin.js` exports: one `app.json` entry for every TV |
| `src/start/` | the dev tools plugin `expo-module.config.json` declares: one `t` key in `expo start` |
| `src/cli/` | the `rn-tv` command |
| `dist/` | the compiled JavaScript npm ships (`bun run build`, run by `prepare`); never edited, never committed |
| `docs/` | the platform package contract, the decisions and the reference |
