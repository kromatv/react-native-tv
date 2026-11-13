# What a TV platform package declares

`@kromatv/react-native-tv` names no platform in its code. It reads the
`package.json` of each dependency of the app (`dependencies`,
`devDependencies`, `optionalDependencies`, `peerDependencies`) and of each of
its own `dependencies`, and takes the ones that have a
`reactNativeTvPlatform`. Its own dependencies are how
`@kromatv/react-native-tizen` and `@kromatv/react-native-webos` come with it;
any other platform joins as a dependency of the app.

Each is looked for from the app first, then from where the app has
`@kromatv/react-native-tv`, so the app's own copy wins and a package only this
one reaches is still found:

```json
{
  "bin": { "rn-webos": "dist/cli/main.js" },
  "exports": {
    "./metro": "./dist/metro/index.js",
    "./package.json": "./package.json"
  },
  "reactNativeTvPlatform": {
    "metro": "./metro",
    "platform": "webos",
    "cli": "rn-webos"
  }
}
```

| Field | Required | What it is |
|---|---|---|
| `metro` | yes | A subpath of the package's `exports`, starting with `./`. Its module exports `withTvPlatform`, a function that takes a Metro config and returns it with the platform added. `withTV` requires it, so it loads synchronously: CommonJS, or an ES module without top-level `await` |
| `platform` | for `run` | The name the platform has in Metro and in `Platform.OS`: lowercase letters and digits, starting with a letter. It names the command, `run:<platform>` |
| `cli` | for `run` | A command of the package's `bin`. `run:<platform> [options]` starts its script with Node as `<script> run [options]`, on the terminal of the caller, and exits with its code |

The package must list `./package.json` in its `exports`, or it is not found,
and `./app.plugin.js`, which Expo is given as a file when the app cannot reach
the package by name.
A package that declares `metro` alone still joins `withTV`; it has no `run`
command until it names `platform` and `cli`.

An app that lists this package's config plugin calls no wrapper: Expo is
handed a Metro config to which each platform that names its `platform` is
added, unless that name is among the config's `resolver.platforms` already.
A platform package that checks the app's Metro config itself before a build
has to load it as Expo CLI does, the app config first and then
`loadUserConfig` with the path in `EXPO_OVERRIDE_METRO_CONFIG`, or it refuses
such an app.

The wrappers run in the order of the package names. When two packages name
the same platform, the first by name gets `run:<platform>`.

## Settings

A platform package reads its settings from its own entry in `expo.plugins`, by
its package name, in the config as Expo evaluates it. `@kromatv/react-native-tv`
adds that entry from its own: the shared fields, then the block named after the
platform.

```json
["@kromatv/react-native-tv", {
  "package": "tv.example.app",
  "locales": ["fr", "en"],
  "tvos": { "androidTVRequired": true },
  "webos": { "splash": "./assets/splash-tv.png" }
}]
```

| Key | Goes to |
|---|---|
| `package`, `version`, `icon`, `locales` | every platform package. A platform's config plugin must take these four under these names |
| a platform's name, such as `webos` | that platform, over the shared fields |
| `tvos` | react-native-tvos' `@react-native-tvos/config-tv`, for Apple TV and Android TV, with `isTV` on |

A platform the app lists itself keeps its own entry, and a block for it is then
an error. A platform gets no entry when neither a shared `package` nor its
block is given.

## `expo start`

The packages of an app meet in one registry inside the `expo start` process,
`globalThis[Symbol.for('reactNativeTvPlatform.start')]`, made by whichever
loads first:

```ts
interface TvStartRegistry {
  claimed: boolean;
  providers: Map<string, TvStartProvider>; // by platform
}

interface TvStartProvider {
  platform: string;
  package: string;
  targets(): Promise<{ target: string; label: string }[]>;
  open(target: string | null): Promise<void>;
}
```

A platform package registers a provider when its dev tools plugin's server
entry loads. `targets` lists what the app can open on, the TV used last first.
`open` opens it on one of them, or on the TV used last for `null`, prints its
own progress and failures, and does not reject.

`@kromatv/react-native-tv` sets `claimed` and adds one `t` and one shift+t for
every provider. A platform package adds its own keys only when `claimed` is
still false at the moment Expo starts reading keys.
