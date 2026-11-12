# Decisions

The choices this package is built on. Change one only by changing this file in
the same change.

## The package holds no platform

It depends on no TV platform package and names none in its code. A package is a
TV platform when its `package.json` has `reactNativeTvPlatform`
([platform-packages.md](platform-packages.md)), and the app's own dependencies
say which platforms it has. A third platform joins by shipping that
declaration, with no release of this package.

`withTV` started in `@kromatv/react-native-tizen` and
`@kromatv/react-native-webos`, each exporting it. An app then imported the
wrapper for every TV from one platform's package, which read as if the other
were not there. The neutral package is where it belongs.

## A platform's `run` is its own command, in its own process

`run:<platform>` starts the platform package's command with `run` and the
arguments as given. This package parses none of them, so a platform's options,
help and exit codes are the platform's, in whatever version the app has
installed. Nothing of a platform is imported into Expo CLI's process.

## Expo CLI learns the commands through a patch the app owns

Expo CLI keeps its commands in one table and gives a package no way to add to
it ([reference/expo-cli.md](reference/expo-cli.md)). `rn-tv expo-patch` adds a
few lines after that table which load this package's commands from the app in
the current directory. The rest is ordinary code in this package, so the patch
changes only when Expo moves the table.

- Where the package manager has a patch feature (Bun, pnpm, Yarn 2 and later),
  the patch is applied with it, never by writing into `node_modules` from an
  install script: the app commits a patch file, sees it in review, and every
  install applies it.
- npm and Yarn 1 have no patch feature, and nothing they keep brings a change
  to an installed file back after an install. There `rn-tv expo-patch` writes
  the lines into the installed file and adds itself to the app's `postinstall`
  script, so the app still commits and reviews what runs, in `package.json`
  instead of a patch file. It differs from the rule above only because the
  alternative is no patch at all for those two managers.
- A patch belongs to one version of `@expo/cli`. `rn-tv expo-patch` removes
  the patch it wrote for another version and never one it did not write.
- It is opt-in. `rn-tv run:<platform>` is the same command without it.
- The lines do nothing in a project that does not depend on this package, and
  a failure inside them warns and leaves Expo's commands as they were: a patch
  must not be able to stop `expo start`.
- A command Expo CLI has already is kept. If Expo ships its own `run:tizen`,
  that one wins.
- Run as an app's `postinstall`, `rn-tv expo-patch` says why it could not
  patch and succeeds all the same. An Expo CLI it does not know yet must not
  fail an install.

## One entry in `app.json` stands for each platform's own

An app on four TVs listed three config plugins and repeated its package id
and its locales in two of them. The plugin of this package takes them once.
It turns its props into the entry each platform package would have had, the
shared fields and then the block named after the platform, applies that
platform's own plugin, and adds the entry to the evaluated config. A
platform's command reads its settings there and cannot tell the difference
([reference/expo-config-and-start.md](reference/expo-config-and-start.md)), so
the platform packages change nothing for this.

- The shared fields are the four every platform takes under one name:
  `package`, `version`, `icon`, `locales`. Anything else is a platform's
  business and goes in its block, as it is.
- This package validates no platform's props. The platform's own plugin does,
  with its own messages.
- `tvos` is react-native-tvos' plugin, the one package named here: it is what
  makes an Expo app an Apple TV and Android TV app, and it is not a platform
  package in this package's sense. Its props pass through as they are, with
  `isTV` on.
- An entry the app lists itself wins, and a block beside it is an error rather
  than a silent merge.

## The `app.json` entry also sets Metro up

A config plugin cannot reach Metro: Expo gives it the app config and nothing
else. But Expo CLI evaluates the app config before it loads the Metro config,
in the same process, and reads the path of a Metro config to load in place of
the app's own from `EXPO_OVERRIDE_METRO_CONFIG`
([reference/expo-config-and-start.md](reference/expo-config-and-start.md)). So
the plugin sets that variable to a Metro config of this package, which loads
the app's own config as Expo would have, then adds the TV platforms. An app
with the entry needs no `withTV`.

- Expo's source calls the variable internal. If Expo drops it, the app's Metro
  config no longer knows the TV platforms and each platform's command stops
  before the build and says so. `withTV` is the way that does not depend on
  it, and stays.
- A Metro config something else put in the variable is left in place.
- Only a platform package that names its platform is added this way. Whether
  the app's own config wrapped a platform already is told from the platform's
  name among the config's platforms, and wrapping one twice would chain its
  transformer to itself.
- `withTV` skips a platform the config knows for the same reason, so an app
  may keep its call.
- It works for whatever runs Expo CLI on the app: `expo start`, a bundle for a
  store build, a platform's own command. A platform's command has to load the
  Metro config as Expo does, the app config first, or its check before a build
  refuses the app.

## One key in `expo start`, owned by this package

Two platform packages in one app each added a `t`, and one key press reached
both. The keys now have one owner. Each platform package registers what it can
open in a registry on `globalThis`
([platform-packages.md](platform-packages.md)); this package claims the keys,
and a platform package that sees the claim adds none.

- This package is a dev tools plugin with a server entry, the one place Expo
  loads a package's code inside `expo start`. The platform packages decide when
  Expo starts reading keys, after every server entry has loaded, so the order
  of loading does not matter.
- It opens nothing itself. `t` and shift+t call the provider of a platform,
  which builds, installs, launches and prints as its own key did.
- `t` goes where the project went last, remembered per project in the cache
  directory. With nothing remembered and several platforms it asks, since no
  platform has a better claim than another.
- A platform package older than the registry keeps its own keys. Nothing
  breaks, and the app has two `t` until that package is updated.

## The package ships JavaScript, and the CLI runs on Node

`bun run build` compiles `src/` to `dist/` with `tsc`, one file per source at
the same depth; `exports` and `bin` point at `dist/`, and `prepare` builds it.
The code uses Node's APIs only. Expo CLI is CommonJS and loads the ES module
`dist/expo-cli/index.js` with `require`, which Node allows from 22.12 on, below
the floor of both this package and Expo CLI (22.13).
