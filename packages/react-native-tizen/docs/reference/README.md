# Platform reference

What react-native-tizen builds on, written down so nobody implements a platform
behaviour from memory. Each file covers one area, cites a source for every fact,
and ends with what the facts mean for this package.

| File | Read it before touching |
|------|------------------------|
| [tizen-tv-platform.md](tizen-tv-platform.md) | the .NET shell, the manifest, privileges, lifecycle, signing, store certification, which TVs a build reaches |
| [tizen-input.md](tizen-input.md) | remote keys, key grabs, Back and Exit, voice, text input and the on-screen keyboard |
| [text-input.md](text-input.md) | `<TextInput>`: Android's events, commands and event counts, and how the host draws and edits a field |
| [tizen-media-playback.md](tizen-media-playback.md) | video: the native player, formats per model year, the video plane, `expo-video` |
| [efl-rendering.md](efl-rendering.md) | anything under `runtime/host/src/views`, `text`, `mounting`: Evas objects, clipping, text, images, transforms, threads |
| [react-native-platform-contract.md](react-native-platform-contract.md) | the host: startup, Fabric mounting, events, TurboModules, TV events, the native animated driver |
| [expo-modules-contract.md](expo-modules-contract.md) | `globalThis.expo`, Expo native modules and views, `expo` startup |
| [intl.md](intl.md) | `Intl`, `localeCompare`, case mapping: Hermes' Intl and its gaps, the ICU compiled in and its data, FormatJS, what Tizen ships |
| [tizen-package-signing.md](tizen-package-signing.md) | `src/signing/`, `src/tizen/signing-identity.ts`, `profiles-file.ts`: which files a signature covers, its XML, the certificates a TV takes, PKCS#12 and `profiles.xml` |
| [expo-cli-integration.md](expo-cli-integration.md) | `app.plugin.js`, `src/config/config-plugin.ts`, `src/devtools/`, `devtools/`, `expo-module.config.json`, `--dev-server`: what `expo install` and `expo start` do with a config plugin and a dev tools plugin, and what Expo does not do for Tizen |
| [tv-discovery.md](tv-discovery.md) | `src/discovery/`: finding Samsung TVs from the computer, SSDP, the TV's REST API and UPnP description, soundbars, the DUID |
| [sdb-protocol.md](sdb-protocol.md) | `src/sdb/`, `src/tizen/devices.ts`, `install-app.ts`: sdb's packets and streams, the services a TV answers, what an install and a launch send |
| [tizen-network.md](tizen-network.md) | local network discovery: mDNS one-shot queries, Tizen's DNS-SD API, what the simulator's network allows |
| [tizen-smart-hub-preview.md](tizen-smart-hub-preview.md) | the tiles above the app on Samsung's home bar: preview JSON, the service app, `PAYLOAD` deep links, `expo.tizen.preview` |
| [tizen-tv-app-integration.md](tizen-tv-app-integration.md) | picture-in-picture and Multi View, playback while hidden (`background-category`, `staysActiveInBackground`), voice through Bixby and `expo.tizen.voice` |

## How a fact is written

- **Cited.** A link to the official page, header or source line it comes from.
  Samsung's and Tizen's own documentation first, then the platform's source,
  then a project that ships on the platform (flutter-tizen).
- **Tagged with how it is known:** documented, inferred (say from what), or
  verified on hardware (say which TV, model year and Tizen version).
- **Scoped to versions** when it differs between Tizen releases or model years.
- **Unknowns stay visible.** "Not documented" is a finding; a guess written as a
  fact is a bug waiting to ship.

The local Tizen headers are documentation too: every public native API is
documented in the rootstrap, for example
`~/tizen-studio/platforms/tizen-9.0/tizen/rootstraps/tizen-9.0-device.core/usr/include`.

## Keeping it true

When hardware disagrees with a page, the hardware wins: record what was seen, on
which TV, and keep the page's claim next to it. When a change relies on a fact
this directory does not have, research it and add it in the same change.
