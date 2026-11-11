# Platform reference

What react-native-webos builds on, written down so nobody implements a platform
behaviour from memory. Each file covers one area, cites a source for every fact,
and ends with what the facts mean for this package.

| File | Read it before touching |
|------|------------------------|
| [webos-tv-platform.md](webos-tv-platform.md) | the .NET shell, the manifest, privileges, lifecycle, signing, store certification, which TVs a build reaches |
| [webos-input.md](webos-input.md) | remote keys, key grabs, Back and Exit, voice, text input and the on-screen keyboard |
| [text-input.md](text-input.md) | `<TextInput>`: Android's events, commands and event counts, and how the host draws and edits a field |
| [webos-media-playback.md](webos-media-playback.md) | video: the native player, formats per model year, the video plane, `expo-video` |
| [efl-rendering.md](efl-rendering.md) | anything under `runtime/host/src/views`, `text`, `mounting`: Evas objects, clipping, text, images, transforms, threads |
| [react-native-platform-contract.md](react-native-platform-contract.md) | the host: startup, Fabric mounting, events, TurboModules, TV events, the native animated driver |
| [react-native-dev-tooling.md](react-native-dev-tooling.md) | `runtime/host/devsupport/`, `--dev-server`: the packager socket, HMR and Fast Refresh, logs in the dev server's terminal, DevSettings, LogBox, reload |
| [expo-modules-contract.md](expo-modules-contract.md) | `globalThis.expo`, Expo native modules and views, `expo` startup |
| [intl.md](intl.md) | `Intl`, `localeCompare`, case mapping: Hermes' Intl and its gaps, the ICU compiled in and its data, FormatJS, what webOS ships |
| [webos-package-signing.md](webos-package-signing.md) | `src/signing/`, `src/webos/signing-identity.ts`, `profiles-file.ts`: which files a signature covers, its XML, the certificates a TV takes, PKCS#12 and `profiles.xml` |
| [expo-cli-integration.md](expo-cli-integration.md) | `app.plugin.js`, `src/config/config-plugin.ts`, `src/commands/init*.ts`, `src/dependencies/`, `src/config/add-webos-plugin.ts`, `src/config/package-id.ts`, `src/metro/metro-platforms.ts`, `src/metro/set-up-metro-config.ts`, `src/devtools/`, `devtools/`, `expo-module.config.json`, `--dev-server`: what `expo install` and `expo start` do with a config plugin and a dev tools plugin, how Expo loads the Metro config, what Expo does not do for webOS, and how each package manager installs one react-native-tvos |
| [tv-discovery.md](tv-discovery.md) | `src/discovery/`: finding Samsung TVs from the computer, SSDP, the TV's REST API and UPnP description, soundbars, the DUID |
| [sdb-protocol.md](sdb-protocol.md) | `src/sdb/`, `src/webos/devices.ts`, `install-app.ts`: sdb's packets and streams, the services a TV answers, what an install and a launch send |
| [webos-network.md](webos-network.md) | local network discovery: mDNS one-shot queries, webOS's DNS-SD API, what the simulator's network allows |
| [webos-smart-hub-preview.md](webos-smart-hub-preview.md) | the tiles above the app on Samsung's home bar: preview JSON, the service app, `PAYLOAD` deep links, `expo.webos.preview` |
| [webos-tv-app-integration.md](webos-tv-app-integration.md) | picture-in-picture and Multi View, playback while hidden (`background-category`, `staysActiveInBackground`), voice through Bixby and `expo.webos.voice` |

## How a fact is written

- **Cited.** A link to the official page, header or source line it comes from.
  Samsung's and webOS's own documentation first, then the platform's source,
  then a project that ships on the platform (flutter-webos).
- **Tagged with how it is known:** documented, inferred (say from what), or
  verified on hardware (say which TV, model year and webOS version).
- **Scoped to versions** when it differs between webOS releases or model years.
- **Unknowns stay visible.** "Not documented" is a finding; a guess written as a
  fact is a bug waiting to ship.

The local webOS headers are documentation too: every public native API is
documented in the rootstrap, for example
`~/webos-studio/platforms/webos-9.0/webos/rootstraps/webos-9.0-device.core/usr/include`.

## Keeping it true

When hardware disagrees with a page, the hardware wins: record what was seen, on
which TV, and keep the page's claim next to it. When a change relies on a fact
this directory does not have, research it and add it in the same change.
