# react-native-tv

<p>
  <a href="https://github.com/kromatv/react-native-tv/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/kromatv/react-native-tv/actions/workflows/ci.yml/badge.svg?branch=main"></a>
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/license-MIT-blue"></a>
  <img alt="React Native 0.88 (react-native-tvos)" src="https://img.shields.io/badge/React%20Native-0.88%20(react--native--tvos)-61dafb">
  <img alt="Expo SDK 58" src="https://img.shields.io/badge/Expo%20SDK-58-000020">
</p>

React Native and Expo apps on TVs, running natively on Hermes and Fabric. One
Expo project builds for Apple TV, Android TV, Samsung Tizen and LG webOS, and
none of them goes through a browser.

## Packages

| Package | What it does | npm |
|---|---|---|
| [`@kromatv/react-native-tv`](packages/react-native-tv) | The one dependency an app adds: it brings the two packages below and sets every TV up with one entry in `app.json`, `run:<platform>` commands in Expo CLI and one `t` key in `expo start` | [![npm](https://img.shields.io/npm/v/@kromatv/react-native-tv?label=npm)](https://www.npmjs.com/package/@kromatv/react-native-tv) |
| [`@kromatv/react-native-tizen`](packages/react-native-tizen) | Samsung TVs. A small .NET app, which Samsung's store accepts, loads a native library with Hermes, React Native's C++ core and a host that draws with EFL | [![npm](https://img.shields.io/npm/v/@kromatv/react-native-tizen?label=npm)](https://www.npmjs.com/package/@kromatv/react-native-tizen) |
| [`@kromatv/react-native-webos`](packages/react-native-webos) | LG TVs. One native executable with Hermes, React Native's C++ core and a host that draws with Skia on OpenGL ES | [![npm](https://img.shields.io/npm/v/@kromatv/react-native-webos?label=npm)](https://www.npmjs.com/package/@kromatv/react-native-webos) |

Apple TV and Android TV come from
[react-native-tvos](https://github.com/react-native-tvos/react-native-tvos),
which `@kromatv/react-native-tv` sets up with the other two.

The two platform packages also work on their own. An app that targets only
Samsung TVs can depend on `@kromatv/react-native-tizen` and nothing else from
here.

## Start

```sh
npm install @kromatv/react-native-tv
```

One entry in `app.json` configures every TV platform and sets Metro up:

```json
{
  "expo": {
    "plugins": [["@kromatv/react-native-tv", { "package": "tv.example.app" }]]
  }
}
```

Then run the app on a TV:

```sh
npx rn-tv run:tizen --device 192.168.1.50
npx rn-tv run:webos --device 192.168.1.51
```

Building for a TV needs that platform's toolchain. The READMEs of
[react-native-tizen](packages/react-native-tizen#readme) and
[react-native-webos](packages/react-native-webos#readme) say what to install,
what works, and which TVs each one has run on.
[react-native-tv](packages/react-native-tv#readme) has every option of the
shared setup.

## Status

All three packages are alphas. The Tizen package runs on a Samsung The Frame
(2024, Tizen 9.0). The webOS package ran on two TVs of LG's Cloud Test Lab
(webOS 24 and webOS 25). Each README lists what was checked on a TV and what
only in a simulator.

## Develop

```sh
git clone https://github.com/kromatv/react-native-tv.git
cd react-native-tv
bun install
bun run typecheck && bun run test && bun run check
```

[CONTRIBUTING.md](CONTRIBUTING.md) covers pull requests and releases.
`AGENTS.md`, here and in each package, has the rules a change follows.

## History

The Tizen and webOS packages were developed in `kromatv/react-native-tizen`
and `kromatv/react-native-webos` until November 2026. Their commits are in this
repository's history, and a reference such as `kromatv/react-native-tizen#84`
in a commit or a changelog points at a pull request of the old repository.

## License

[MIT](LICENSE)
