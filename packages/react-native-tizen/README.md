<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/kromatv/react-native-tizen/main/docs/assets/wordmark-dark.svg">
    <img alt="react-native-tizen" src="https://raw.githubusercontent.com/kromatv/react-native-tizen/main/docs/assets/wordmark-light.svg" width="480">
  </picture>
</p>

<p align="center">
  React Native and Expo apps on Samsung TVs, running natively on Hermes and Fabric.
</p>

<p align="center">
  <a href="https://www.npmjs.com/package/@kromatv/react-native-tizen"><img alt="npm" src="https://img.shields.io/npm/v/@kromatv/react-native-tizen/next?label=npm"></a>
  <a href="https://github.com/kromatv/react-native-tizen/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/kromatv/react-native-tizen/actions/workflows/ci.yml/badge.svg?branch=main"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=kromatv_react-native-tizen"><img alt="Quality gate" src="https://sonarcloud.io/api/project_badges/measure?project=kromatv_react-native-tizen&metric=alert_status"></a>
  <a href="https://sonarcloud.io/component_measures?id=kromatv_react-native-tizen&metric=coverage"><img alt="Coverage" src="https://sonarcloud.io/api/project_badges/measure?project=kromatv_react-native-tizen&metric=coverage"></a>
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/license-MIT-blue"></a>
  <br>
  <img alt="React Native 0.88 (react-native-tvos)" src="https://img.shields.io/badge/React%20Native-0.88%20(react--native--tvos)-61dafb">
  <img alt="Expo SDK 58" src="https://img.shields.io/badge/Expo%20SDK-58-000020">
  <img alt="Tizen TV 6.0 to 10.0, verified on 9.0" src="https://img.shields.io/badge/Tizen%20TV-6.0%20to%2010.0%2C%20verified%20on%209.0-1428a0">
</p>

<p align="center">
  <img alt="The example app on a Samsung TV: the title React Native, native on a Samsung TV, and three cards for Hermes, Fabric and Expo" src="https://raw.githubusercontent.com/kromatv/react-native-tizen/main/docs/assets/screenshot-home.png" width="720">
  <br>
  <sub>The example app on a Samsung The Frame (2024), Tizen 9.0, captured by the app itself with <code>rn-tizen screenshot</code>, which never includes video.</sub>
</p>

## Why

Samsung's TV store takes two kinds of apps: web apps and .NET apps. A React
Native app reaches a Samsung TV today as a web app: react-native-web renders
it to the DOM, in the TV's browser engine. That is a second renderer, with its
own layout, input and performance, to build and test for.

react-native-tizen runs the app the way Android and iOS run it. A small .NET
app, which the store accepts, loads `libreact-native-tizen.so`. That library
holds Hermes, React Native's C++ core (Fabric, TurboModules, Yoga) and a Tizen
host that draws with EFL, the TV's own graphics toolkit. Your JavaScript ships
as Hermes bytecode. There is no browser and no WebView in the process.

For an Expo app, Tizen is one more platform. The same project and the same
`app.json` build for it, and `Platform.OS` is `'tizen'`.

## Features

- **Expo integration.** `npx expo install` adds the config plugin, and
  `npx expo start` lists Run on TV, Run in simulator, Find TVs and Create
  certificate under shift+m.
- **Development builds.** Fast Refresh, `r` to reload, `console.log` in the
  Expo terminal, LogBox on the TV, and React Native DevTools on the TV's Hermes
  with `j`. `rn-tizen screenshot` saves what a development build draws, on a
  retail TV too. It captures the app's window only: video plays on a plane
  under it, so a video's area comes out transparent.
- **The remote, as on Apple TV.** `useTVEventHandler` gets one event per
  press, a key held for 300 ms sends `longSelect` or `longUp`, and Back reaches
  `BackHandler`.
- **Video.** `expo-video` plays on the TV's hardware player, with audio and
  subtitle track selection.
- **Samsung TV features.** Voice commands and voice search, Smart Hub preview
  tiles, background audio, and a 3840x2160 UI plane on TVs that grant one. Each
  is one prop in `app.json`.
- **Expo modules.** `expo-image`, `expo-video`, `expo-file-system`,
  `expo-font`, `expo-blur`, `expo-device`, `expo-constants`, `expo-keep-awake`
  and `expo/fetch` have Tizen implementations.
- **Native libraries.** A library with a `codegenConfig` compiles into the
  runtime. react-native-svg ships with a Tizen implementation.
- **`Intl`.** ICU 78.3 is compiled in, with its data trimmed to the locales you
  list: 2.49 MB for English.
- **No Tizen Studio tools at run time.** `rn-tizen` speaks sdb to the TV, signs
  `.tpk` and `.wgt` packages in TypeScript and finds TVs on the network in about
  five seconds. The same code is a Node API.
- **A simulator.** The same runtime, built for aarch64 Linux, runs in a VM on
  an Apple silicon Mac, with screenshots, remote keys and native backtraces. It
  has no media player, so it plays no video.

## Quick start

### 1. Install the tools

macOS on Apple silicon is the only build host verified so far.

- [Node.js](https://nodejs.org) 22.13 or newer, which runs the CLI and Expo.
- [Tizen Studio](https://developer.tizen.org/development/tizen-studio/download)
  with, from its Package Manager, `IOT-Headed-6.0-NativeAppDevelopment-CLI` (the
  Tizen 6.0 rootstrap the native build links against), `NativeToolchain-Gcc-14.2`
  (the GCC runtime it links) and the Samsung Certificate Extension. A package
  built against 6.0 runs on every later Tizen. Another floor takes its own
  rootstrap package, which `npx rn-tizen init` and `npx rn-tizen doctor` name.
  `rn-tizen` never runs Tizen Studio's `sdb` or `tizen` tools: it reaches the
  TV and signs the package itself.
- LLVM, CMake, Ninja and .NET:

  ```sh
  brew install llvm lld cmake ninja dotnet ccache
  ```

### 2. Put the TV in Developer mode

On the TV, open Apps, type `12345` on the remote, turn Developer mode on with
your computer's IP address, then restart the TV.

### 3. Add the package

In an Expo SDK 58 app, point `react-native` at
[react-native-tvos](https://github.com/react-native-tvos/react-native-tvos), the
TV fork of React Native this package is built and tested against. It brings
`useTVEventHandler` and the other TV APIs. In `package.json`:

```json
{
  "dependencies": {
    "react-native": "npm:react-native-tvos@0.88.0-0"
  },
  "overrides": {
    "react-native": "$react-native"
  }
}
```

The override keeps a single React Native in the app. react-native-tvos's own
list package asks for `react-native` 0.88.0, so without it npm installs stock
React Native beside the fork, and the app stops on the TV with `"main" has not
been registered`. With Yarn or pnpm, put `npm:react-native-tvos@0.88.0-0` under
`resolutions` or `pnpm.overrides` instead.

Then install, add the package and set the app up:

```sh
npm install
npx expo install @kromatv/react-native-tizen
npx rn-tizen init
```

The commands here use npm's `npx`. With Bun, pnpm or Yarn, run `rn-tizen` with
`bunx`, `pnpm exec` or `yarn` instead, as `init` prints them. In a Bun
workspace `npx rn-tizen` answers 404: npm finds no bin at the workspace root,
where Bun does not link it, and asks the npm registry for a package of that
name.

`rn-tizen init` gives the config plugin Expo listed in `app.json` your app's
Tizen package id. It asks for one, suggesting your Android package or one made
from the slug. It also writes `metro.config.mjs`, which teaches Metro the
`tizen` platform:

```js
import { getDefaultConfig } from 'expo/metro-config';
import { withTizen } from '@kromatv/react-native-tizen/metro';
export default withTizen(getDefaultConfig(import.meta.dirname));
```

An app that already has a Metro config keeps it: `init` wraps its export with
`withTizen`, or prints the two lines to add when it cannot change the file
safely.

Last, `init` points `react-native` at
[react-native-tvos](https://github.com/react-native-tvos/react-native-tvos),
which brings `useTVEventHandler` and the other TV APIs, adds the override that
keeps every other package on that copy, and runs your package manager's
install. Running `init` again changes nothing.

### 4. Get a Samsung certificate for your TV, once

```sh
npx rn-tizen devices
```

lists the Samsung TVs on your network with each one's DUID. In Tizen Studio,
Tools, Certificate Manager, create a Samsung profile with that DUID and make it
the active profile. Samsung issues both its certificates, and a retail TV takes
no other. `rn-tizen` reads the profile from there.

### 5. Run it from `expo start`

```sh
npx expo start
```

Press **shift+m** ("more tools") and pick `@kromatv/react-native-tizen`. Expo
shows the command each entry runs and asks "(y/N)" before running it; answer
`y`.

- **Run on TV** builds a development build, installs it on the TV and launches
  it. It loads its JavaScript from the dev server you are running. Give it the
  TV's IP address the first time; later, leave the address empty to use the TV
  you used last.
- **Run in simulator** does the same on the virtual TV of
  [the simulator](simulator/README.md).
- **Find TVs** lists the Samsung TVs on your network, with their Developer
  mode, sdb and DUID.
- **Create certificate** makes an author certificate.

Each command prints the address of a page inside the dev server, where you can
follow its progress and logs. The first run builds the native runtime, which
takes a few minutes; later runs only rebuild what changed. Run on TV and Run
in simulator check first that the app loads one `react-native` and that its
Metro config has `withTizen`, and say to run `npx rn-tizen init` when not. `npx rn-tizen doctor` names the exact fix for
anything a build needs.

Expo has no `expo run:tizen` and no single-key shortcut for Tizen, and
`expo prebuild` and `expo export --platform tizen` do not know the platform.
For a script beside Expo's own, add one to `package.json`:

```json
{
  "scripts": {
    "android": "expo start --android",
    "tizen": "rn-tizen run"
  }
}
```

`npm run tizen` packages the app with its JavaScript inside, installs it on the
only TV used before, launches it and prints its logs until Ctrl+C.

Store packages need a release build, which `package` makes; `--dev` makes a
development one that streams its logs:

```sh
npx rn-tizen package
npx rn-tizen package --dev
```

Publishing to Samsung's store needs a Seller Office partner group, because the
package carries a native library.

## Examples

Each one compiles against the package's API and react-native-tvos's types. The
[example app](example/) has a screen for each.

### Remote keys

```tsx
import { useEffect, useState } from 'react';
import { BackHandler, Text, useTVEventHandler } from 'react-native';

export function LastKey() {
  const [key, setKey] = useState('none');
  useTVEventHandler((event) => setKey(event.eventType));
  useEffect(() => {
    const subscription = BackHandler.addEventListener('hardwareBackPress', () => {
      setKey('back');
      return true;
    });
    return () => subscription.remove();
  }, []);
  return <Text>{`Last key: ${key}`}</Text>;
}
```

`eventType` is `up`, `down`, `left`, `right`, `select`, `playPause`,
`fastForward`, `rewind`, a digit, a color key, or `long<Key>` for a hold. A Back
handler that returns `false` lets the app exit.

### Focus

Tizen has no native focus engine yet: `focusable`, `hasTVPreferredFocus` and
`TVFocusGuideView` do nothing. Keep the focused item in state and move it on
remote events:

```tsx
import { useState } from 'react';
import { StyleSheet, Text, View, useTVEventHandler } from 'react-native';

const ITEMS = ['Home', 'Movies', 'Shows', 'Settings'];

export function Menu({ onOpen }: { onOpen: (item: string) => void }) {
  const [focused, setFocused] = useState(0);
  useTVEventHandler((event) => {
    if (event.eventType === 'right') setFocused((index) => Math.min(ITEMS.length - 1, index + 1));
    if (event.eventType === 'left') setFocused((index) => Math.max(0, index - 1));
    if (event.eventType === 'select') onOpen(ITEMS[focused]!);
  });
  return (
    <View style={styles.row}>
      {ITEMS.map((item, index) => (
        <View key={item} style={[styles.item, index === focused && styles.focused]}>
          <Text style={styles.label}>{item}</Text>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 24 },
  item: { paddingHorizontal: 32, paddingVertical: 16, borderRadius: 12 },
  focused: { backgroundColor: '#3d5afe' },
  label: { color: '#fff', fontSize: 32 },
});
```

### Video

```tsx
import { useVideoPlayer, VideoView } from 'expo-video';
import { useTVEventHandler } from 'react-native';

export function Player({ uri }: { uri: string }) {
  const player = useVideoPlayer(uri, (created) => created.play());
  useTVEventHandler((event) => {
    if (event.eventType === 'playPause') {
      if (player.playing) player.pause();
      else player.play();
    }
    if (event.eventType === 'fastForward') player.seekBy(30);
    if (event.eventType === 'rewind') player.seekBy(-10);
  });
  return <VideoView player={player} contentFit="contain" style={{ flex: 1 }} />;
}
```

The TV draws video on its own plane, under the UI, so `rn-tizen screenshot`
shows the video's area transparent. What expo-video does on Tizen, call by
call, is in
[the media reference](docs/reference/tizen-media-playback.md#7-what-react-native-tizen-implements).

### Voice

With `"voice": { "searchUrl": "myapp://search?q={query}" }` in the plugin's
props, "pause", "skip forward 30 seconds" and "select Dune" work with no code.
To handle a search yourself:

```tsx
import { Voice } from '@kromatv/react-native-tizen/voice';
import { useEffect } from 'react';

export function useVoiceSearch(openSearch: (query: string) => void) {
  useEffect(() => {
    Voice.setScreen('home');
    const search = Voice.addListener('search', (query) => openSearch(query.title ?? query.utterance));
    return () => search.remove();
  }, [openSearch]);
}
```

### Smart Hub preview

With `"preview": { "refreshMinutes": 30 }` in the plugin's props, the app
publishes the tiles Samsung's home bar shows above its icon. Pressing a tile
opens the app with its `url` through `Linking`.

```ts
import { SmartHubPreview } from '@kromatv/react-native-tizen/preview';

export async function publishContinueWatching() {
  const result = await SmartHubPreview.set([
    {
      title: 'Continue watching',
      tiles: [
        {
          title: 'Blade Runner',
          subtitle: '1 h 12 left',
          image: 'https://example.com/posters/blade-runner-640x360.jpg',
          ratio: '16:9',
          url: 'myapp://item/42',
          playable: true,
        },
      ],
    },
  ]);
  if (!result.published) console.warn(result.reason);
}
```

Off a Samsung TV, `Voice` and `SmartHubPreview` answer "not available", so the
same code runs on Android, iOS and the web. Samsung documents voice and the
personal preview for web apps only; ask your Seller Office content manager
before you ship either to the store. See [TV features](docs/tv-features.md).

## Benchmarks

Measured on a Samsung GQ75LS03DAUXZG (The Frame, 2024, Tizen 9.0, armv7), with
the example app's benchmark screen: a 1,000-row FlatList and 12 views on the
native animated driver. Each figure is a median.

| What | Result | Runs |
|---|---|---|
| Cold start, launch command to first frame (release) | 1,196 ms | 17 launches |
| The same screen as a Tizen web app (react-native-web), same launches | 2,075 ms | 17 launches |
| FlatList, focus moving one row every 200 ms | 54.1 frames/s, 22 dropped frames in 114 s | 3 runs |
| Native-driver animation | 49.9 frames/s at the TV's 50 Hz, 0 dropped | 3 runs |
| Resident memory (release), list still / scrolling | 63.7 MB / 94.4 MB | 3 runs |
| `.tpk` size (release) | 8.0 MB, against 0.16 MB for the web app | 1 build |

<p align="center">
  <img alt="The benchmark screen on the TV: a FlatList of 1,000 rows scrolling, row 75 focused, resident memory 87.2 MB" src="https://raw.githubusercontent.com/kromatv/react-native-tizen/main/docs/assets/screenshot-flatlist.png" width="640">
</p>

Frame counts come from a development build, the only kind `rn-tizen perf`
reads. At one row every 50 ms the JavaScript thread saturates and frames drop.
The web app was compared on cold start and size only: a web page cannot read
the canvas's frame counter or its own process's memory. The method, every run
and the commands to reproduce them are in [docs/benchmarks.md](docs/benchmarks.md).

## Status

This is an alpha. `rn-tizen` builds for Tizen 6.0 to 10.0, Samsung's TVs from
2021 on: a package reaches its floor, 6.0 by default, and every later version
([Tizen versions](docs/tizen-versions.md)). One TV is verified on hardware so
far: a Samsung GQ75LS03DAUXZG (The Frame, 2024) on Tizen 9.0, armv7.

Known gaps:

- Only Tizen 9.0 has run on a TV. A 6.0-floor package runs there, and the
  runtime's imports resolve in every rootstrap from 6.0 to 10.0, but no TV on
  another version has run it.
- TVs from 2020 and earlier (Tizen 5.5 and below) cannot run it: Samsung blocks
  the native libraries an app carries below 6.0
  ([reference](docs/reference/tizen-tv-platform.md#2-model-year-platform-runtime)).
- No native focus engine, as above.
- A `FlatList` sized only by `flex: 1` draws nothing. On a TV,
  react-native-tvos wraps every virtualized list in a `TVFocusGuideView` that has
  no flex, so the list collapses to 1 px. Give the list a height.
- react-native-svg draws no text, images, masks or clip paths yet
  ([details](docs/native-modules.md#react-native-svg)).
- A native library cannot declare Tizen privileges, and an app cannot leave a
  library out of the runtime ([limits](docs/native-modules.md#limits)).
- `expo prebuild`, `expo export --platform tizen` and `expo run:tizen` do not
  exist: Expo's CLI has no way to add a platform. `rn-tizen package` and
  `rn-tizen run` stand in.

## Documentation

**Getting started**

- [Quick start](#quick-start), above
- [The simulator](simulator/README.md): a virtual TV on an Apple silicon Mac
- [Tizen versions](docs/tizen-versions.md): which Samsung TVs a package
  reaches, and why the floor is 6.0
- [Benchmarks](docs/benchmarks.md): the method and the raw results

**CLI**

- [The `rn-tizen` CLI](docs/cli.md): every command and option, the config
  plugin's props, and where builds land
- [The `tizen` Node API](docs/tizen-api.md): find TVs, sign and install packages
  from your own tools
- [Runtime config](docs/runtime-config.md): what the runtime reads at startup

**Expo**

- [Inside `npx expo start`](docs/cli.md#inside-npx-expo-start): the shift+m
  commands
- [Develop with `expo start`](docs/cli.md#develop-with-expo-start): Fast
  Refresh, reload, logs, LogBox, DevTools
- [Native libraries and Expo modules](docs/native-modules.md): what compiles into
  the runtime, and how a library adds Tizen support
- [Expo CLI integration](docs/reference/expo-cli-integration.md) and
  [the Expo modules contract](docs/reference/expo-modules-contract.md)

**TV features**

- [Voice, Smart Hub preview, background audio and 4K](docs/tv-features.md)
- [Remote keys and text input](docs/reference/tizen-input.md),
  [`<TextInput>`](docs/reference/text-input.md)
- [Video](docs/reference/tizen-media-playback.md)
- [App integration](docs/reference/tizen-tv-app-integration.md) and
  [the Smart Hub preview](docs/reference/tizen-smart-hub-preview.md)

**Platform reference**

- [Design decisions](docs/decisions.md): what the package is built on, and why
- [The reference index](docs/reference/README.md), with sources for every fact:
  [the TV platform](docs/reference/tizen-tv-platform.md),
  [EFL rendering](docs/reference/efl-rendering.md),
  [React Native's platform contract](docs/reference/react-native-platform-contract.md),
  [dev tooling](docs/reference/react-native-dev-tooling.md),
  [`Intl`](docs/reference/intl.md),
  [networking](docs/reference/tizen-network.md),
  [the sdb protocol](docs/reference/sdb-protocol.md),
  [package signing](docs/reference/tizen-package-signing.md),
  [TV discovery](docs/reference/tv-discovery.md)

**Contributing**

- [Contributing](CONTRIBUTING.md): set up, the loop, the checks
- [Security policy](SECURITY.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

CI runs the TypeScript tests on Linux and macOS and installs the packed package
into a fresh Expo app under Node. When the runtime changes, it builds it for
aarch64 Linux and runs its C++ tests (doctest). SonarCloud's quality gate checks
every pull request, and mutation testing runs weekly.

## Licence

MIT. What the runtime and the packages it builds carry from other projects,
and under which licences, is in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

Not affiliated with or endorsed by Samsung, the Linux Foundation, Meta or Expo.
Samsung, Tizen, Smart Hub, React Native and Expo are trademarks of their owners.
