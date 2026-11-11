# Platform reference

What react-native-webos builds on, written down so nobody implements a platform
behaviour from memory. Each file covers one area, cites a source for every fact,
and ends with what the facts mean for this package.

| File | Read it before touching |
|------|------------------------|
| [webos-tv-platform.md](webos-tv-platform.md) | app types and store policy, `appinfo.json`, how a native app starts, lives and dies, the jail and its paths, the Luna bus, which TVs a build reaches, the screen |
| [hardware-verification.md](hardware-verification.md) | claiming that something works on a TV: which TVs ran which build, what was pressed and what the screen showed |
| [webos-ipk-format.md](webos-ipk-format.md) | `src/package/`: the `.ipk` container, its control file and data tree, what a TV's installer needs |
| [webos-native-toolchain.md](webos-native-toolchain.md) | `runtime/cmake/toolchain/`, `runtime/compat/`, `src/sysroot/`: the sysroot, the ABI, the glibc floor, what is safe to link from the TV and what must be compiled in |
| [webos-device-protocol.md](webos-device-protocol.md) | `src/webos/`: Developer Mode, the key server, SSH as `prisoner`, the Luna commands an install, a launch and a removal send |
| [webos-remote-control.md](webos-remote-control.md) | `src/remote/`: SSAP pairing, remote buttons, text, screenshots |
| [tv-discovery.md](tv-discovery.md) | `src/discovery/`: finding LG TVs from the computer, SSDP, what identifies a TV and its webOS version |
| [webos-window-input.md](webos-window-input.md) | the window and the OpenGL ES context, SDL, remote keys, Back and Exit, the pointer, the on-screen keyboard |
| [skia-rendering.md](skia-rendering.md) | anything under `runtime/host/src/render`, `views`, `text`, `mounting`: Skia's build, Ganesh on OpenGL ES 2, SkParagraph, fonts, ICU break data, performance on a TV's GPU |
| [text-input.md](text-input.md) | `<TextInput>`: Android's events, commands and event counts, and how the host draws and edits a field |
| [webos-media-playback.md](webos-media-playback.md) | video: the TV's media pipeline, the video plane and the hole in the UI, formats, `expo-video` |
| [react-native-platform-contract.md](react-native-platform-contract.md) | the host: startup, Fabric mounting, events, TurboModules, TV events, the native animated driver |
| [react-native-dev-tooling.md](react-native-dev-tooling.md) | `runtime/host/devsupport/`, `--dev-server`: the packager socket, HMR and Fast Refresh, logs in the dev server's terminal, DevSettings, LogBox, reload |
| [expo-modules-contract.md](expo-modules-contract.md) | `globalThis.expo`, Expo native modules and views, `expo` startup |
| [intl.md](intl.md) | `Intl`, `localeCompare`, case mapping: Hermes' Intl and its gaps, the ICU compiled in and its data, FormatJS |
| [expo-cli-integration.md](expo-cli-integration.md) | `app.plugin.js`, `src/config/`, `src/commands/init*.ts`, `src/dependencies/`, `src/metro/`, `src/devtools/`, `devtools/`, `expo-module.config.json`: what `expo install` and `expo start` do with a config plugin and a dev tools plugin, how Expo loads the Metro config, what Expo does not do for webOS |

`react-native-platform-contract.md`, `react-native-dev-tooling.md`,
`expo-modules-contract.md`, `expo-cli-integration.md` and `intl.md` came with
the port from react-native-tizen: what they say about React Native, Expo,
Hermes and ICU holds here, a row that names Tizen, EFL or a Samsung TV
describes that platform, and an observation dated before 2026-11-11 was made
with react-native-tizen, with only the names changed.

## How a fact is written

- **Cited.** A link to the official page, header or source line it comes from.
  LG's own documentation and webOS OSE's sources first, then the source of a
  project that ships on webOS TVs (SDL-webOS, Kodi, moonlight-tv, LG's Flutter
  tool), then community documentation (webosbrew, openlgtv).
- **Tagged with how it is known:** documented, read from source, inferred (say
  from what), or verified on hardware (say which TV, model year and webOS
  version).
- **Scoped to versions** when it differs between webOS releases or model years.
- **Unknowns stay visible.** "Not documented" is a finding; a guess written as a
  fact is a bug waiting to ship.

No fact here is verified on hardware until a row says so. The port was made
without an LG TV at hand; what has run on one since is in
[hardware-verification.md](hardware-verification.md), and each file lists what
only a TV can settle under "Open questions".

## Keeping it true

When hardware disagrees with a page, the hardware wins: record what was seen, on
which TV, and keep the page's claim next to it. When a change relies on a fact
this directory does not have, research it and add it in the same change.
