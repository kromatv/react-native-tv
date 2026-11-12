# @kromatv/react-native-tv

One setup for an Expo app on every TV. Apple TV and Android TV come from
[react-native-tvos](https://github.com/react-native-tvos/react-native-tvos),
Samsung Tizen from
[`@kromatv/react-native-tizen`](https://github.com/kromatv/react-native-tizen)
and LG webOS from
[`@kromatv/react-native-webos`](https://github.com/kromatv/react-native-webos).
This package is what an app sets up once for all of them. It holds no platform
code and depends on none of them: it finds the TV platform packages among the
app's dependencies.

Not published yet.

## Metro

```js
// metro.config.mjs
import { getDefaultConfig } from 'expo/metro-config';
import { withTV } from '@kromatv/react-native-tv/metro';

export default withTV(getDefaultConfig(import.meta.dirname));
```

`withTV` teaches Metro the platform of every TV platform package the app
depends on. Adding a TV platform is installing its package: the Metro config
does not change.

## One entry in `app.json`

```json
{
  "expo": {
    "plugins": [
      [
        "@kromatv/react-native-tv",
        {
          "package": "tv.example.app",
          "locales": ["fr", "en"],
          "tvos": { "androidTVRequired": true },
          "tizen": { "preview": { "refreshMinutes": 30 } }
        }
      ]
    ]
  }
}
```

| Key | Goes to |
|---|---|
| `package`, `version`, `icon`, `locales` | every TV platform package of the app |
| a platform's name: `tizen`, `webos` | that platform's own props, over the shared ones |
| `tvos` | `@react-native-tvos/config-tv`, for Apple TV and Android TV, with `isTV` on, so no script needs `EXPO_TV=1` |

Each platform package still validates its own props, and its command reads
them as if the app had listed its plugin itself. A platform the app does list
itself keeps that entry.

## Run on a TV

```sh
npx rn-tv run:tizen --device 192.168.1.50
npx rn-tv run:webos --device 192.168.1.51
npx rn-tv platforms
```

`run:<platform>` is the platform's own `run`, with its own options:
`rn-tv run:tizen --help` prints the help of `rn-tizen run`.

## The same commands in Expo CLI

Expo CLI has `run:ios` and `run:android` and no way for a package to add a
command. `rn-tv expo-patch` adds one, as a patch of `@expo/cli` that the app
commits:

```sh
npx rn-tv expo-patch
npx expo run:tizen --device 192.168.1.50
npx expo run:webos
npx expo run tizen
```

The patch is a few lines in the file that holds Expo's command table. They load
this package's commands when the app in the current directory depends on it,
and do nothing anywhere else. Expo's own commands are untouched, and
`npx expo --help` lists the new ones.

- With Bun, pnpm and Yarn 2 or later it is written with the package manager's
  own patch feature, and every install applies it. `rn-tv expo-patch` names the
  files to commit:

  | Manager | Patch file | Entry |
  |---|---|---|
  | Bun | `patches/@expo%2Fcli@<version>.patch` | `patchedDependencies` in `package.json` |
  | pnpm | `patches/@expo__cli@<version>.patch` | `patchedDependencies` in `pnpm-workspace.yaml` |
  | Yarn 2 and later | `.yarn/patches/@expo-cli-npm-<version>-<hash>.patch` | `resolutions` in `package.json` |

- npm and Yarn 1 have no patch feature. There `rn-tv expo-patch` writes the
  lines into the installed file and adds itself to the app's `postinstall`
  script, so an install puts them back. Commit `package.json`. In a workspace,
  an install from the root runs the root's `postinstall` only: run
  `rn-tv expo-patch` from the app after it.
- It belongs to one version of `@expo/cli`. After an Expo upgrade that changes
  it, run `rn-tv expo-patch` again: it removes the patch it wrote for the old
  version and writes the new one. Two managers need more:
  - pnpm refuses to install while a patch is for a version that is gone, so
    remove it before the upgrade: `pnpm patch-remove @expo/cli@<old version>`.
    `rn-tv expo-patch` prints that command.
  - npm runs no `postinstall` after `npm install <package>`, so an upgrade
    made that way leaves Expo CLI unpatched until the next `npm install` or
    `rn-tv expo-patch`.
- Without it nothing is lost: `rn-tv run:<platform>` does the same.

## One key in `expo start`

With this package in an app, `npx expo start` has one `t` for every TV:

| Key | Does |
|---|---|
| **t** | Opens the app where this project opened last. With nothing remembered, an app with one platform opens on that platform's TV used last, and an app with several asks |
| **shift+t** | Lists the TVs and simulators of every platform in one list, and opens the app on the one whose number you press; Esc cancels |

It needs versions of the platform packages that register with it. An older one
keeps its own `t`.

## What a platform package declares

A package is a TV platform when its `package.json` says so:

```json
{
  "bin": { "rn-webos": "dist/cli/main.js" },
  "reactNativeTvPlatform": {
    "metro": "./metro",
    "platform": "webos",
    "cli": "rn-webos"
  }
}
```

[`docs/platform-packages.md`](docs/platform-packages.md) has the contract.

## Not there yet

- `init` and `doctor` commands: an app is set up by hand, as above.
- `expo start --tizen` and the like, to open on a TV as the dev server starts.
- The Expo CLI patch on Windows, and with pnpm before 11: neither has been
  run.
- With npm or Yarn 1 in a workspace, the `postinstall` of the workspace root.
  `rn-tv expo-patch` adds itself to the app's script only.
