# Expo config plugins and `expo start`: one entry, one key

What Expo does with a config plugin that stands for other plugins, how the TV
platform packages find their settings, and what a dev tools plugin can do
inside `expo start`. Checked on 2026-11-12.

| Tag | Meaning |
|---|---|
| **S** | Read in source, under `build/`: `@expo/config` 58.0.12, `@expo/config-plugins` 58.0.13, `@expo/cli` 58.1.19, `@react-native-tvos/config-tv` 0.1.7, and `@kromatv/react-native-webos` at its commit `3568aa2` |
| **O** | Observed on 2026-11-12, on an arm64 Mac with Node 26.5.0 and Bun 1.4.2: an app in a temporary directory with `expo` 58.0.23 (`@expo/cli` 58.1.20), `react-native-tvos` 0.88.0-0, `@react-native-tvos/config-tv` 0.1.7, this package from its packed tarball, and `@kromatv/react-native-tizen` and `@kromatv/react-native-webos` packed from their branches that declare `platform` and `cli` and register with the start registry |

## 1. A plugin that stands for other plugins

| Fact | Tag | Source |
|---|---|---|
| `getConfig` sets `exp._internal.projectRoot` before it applies the plugins, and removes `_internal` from a public config only. | S | `@expo/config` `build/Config.js:199-230` |
| The plugins are applied as `withPlugins(config, config.plugins)`: a `reduce` over the list as it is at that moment, each entry resolved by name from the project root. An entry a plugin adds to the config it returns is therefore kept in the result and not applied by Expo. | S | `@expo/config` `build/plugins/withConfigPlugins.js:19-34`; `@expo/config-plugins` `build/plugins/withPlugins.js:24-29` |
| A plugin that returned its config with `["@kromatv/react-native-webos", {...}]` appended to `plugins` gave `getConfig(root).exp.plugins` both entries, its own and the appended one. | O | |
| `rn-webos` reads its settings from the props of the entry named `@kromatv/react-native-webos` in `exp.plugins`, of the config as `getConfig` evaluates it, plugins applied. `rn-tizen` does the same with its own name. | S | react-native-webos `src/app/expo-config.ts:10-26`, `src/config/webos-settings.ts:16-28` |
| The config plugin of each platform package checks its props and changes nothing in the config. | S | react-native-webos `src/config/config-plugin.ts:9-14` |
| Both platform packages take `package`, `version`, `icon` and `locales` under those names; the title a TV shows defaults to `expo.name`. | S | react-native-webos `src/config/webos-config.ts:44-56, 127`; react-native-tizen at `35056e6`, `src/config/tizen-config.ts:42-55` |
| With one entry, `["@kromatv/react-native-tv", { "package": ..., "locales": [...], "tvos": { "androidTVRequired": true }, "tizen": { "preview": {...} } }]`, `exp.plugins` held that entry, then one for `@kromatv/react-native-tizen` and one for `@kromatv/react-native-webos`, each with the shared fields and its own block. `expo run:webos` and `expo run:tizen` then went past their settings to the TV. | O | |
| react-native-tvos' plugin builds for TV when `EXPO_TV` is set or its `isTV` prop is true: `env.EXPO_TV \|\| (params?.isTV ?? false)`. | S | `@react-native-tvos/config-tv` `build/utils/config.js:11-23` |
| With that entry, `expo prebuild --platform android --no-install` wrote `<uses-feature android:name="android.software.leanback" android:required="true"/>` and the `LEANBACK_LAUNCHER` category into `AndroidManifest.xml`, with no `EXPO_TV` in the environment. | O | |
| A block named after no platform of the app stopped `expo config` with this package's message. | O | |

## 2. A dev tools plugin with a server entry and nothing else

| Fact | Tag | Source |
|---|---|---|
| A dev tools plugin needs `packageName` and `packageRoot` only; `webpageRoot`, `serverEntryPoint` and `cliExtensions` are each optional. | S | `@expo/cli` `build/src/start/server/devtools/DevToolsPlugin.schema.js:99-129` |
| Every plugin's `serverEntryPoint` is loaded as Metro starts, before the terminal UI. | S | `build/src/start/server/metro/instantiateMetro.js:389-404` |
| A plugin with a page or a server entry has an endpoint, `/_expo/plugins/<package>`, and shift+m lists it as `Open <package>`. | S | `build/src/start/server/devtools/DevToolsPlugin.js:59-61`; `build/src/start/interface/createDevToolsMenuItems.js:95-96, 104-116` |

## 3. The keys

| Fact | Tag | Source |
|---|---|---|
| With the released platform packages, 0.1.0-alpha.4 and 0.1.0-alpha.2 with only `platform` and `cli` added, `expo start` printed `› Press t │ open on Tizen TV` and `› Press t │ open on webOS TV`, and one shift+t both opened the webOS list and started the Tizen search: each package listens on its own. | O | |
| With the branches that register with the start registry, and this package's plugin: one line, `› Press t │ open on TV`; `?` added that line and `› shift+t │ select a TV or a simulator` once; shift+t printed one list, a Samsung TV connected before, the Tizen simulator and the webOS simulator, each with its platform in front; Esc closed it and Expo's keys worked after. | O | |

## What this means for this package

- The config plugin turns its props into one entry per TV platform package and
  one for react-native-tvos' plugin, applies them with `withPlugins` from the
  app's own `expo/config-plugins`, and appends the platform entries to
  `plugins`, where each platform's command finds its settings as if the app
  had listed them. It needs no change in the platform packages beyond the
  `platform` they declare.
- The shared fields are the four both platforms take under one name. Anything
  else belongs in a platform's block, which is passed on as it is: this
  package validates no platform's props, the platform's own plugin does.
- `tvos` is passed to react-native-tvos' plugin as it is, with `isTV` on unless
  the block says otherwise, so an app needs no `EXPO_TV` in its scripts.
- This package is a dev tools plugin with a server entry only, the one place
  Expo loads a package's code inside `expo start`. It claims the keys there.
  Since shift+m lists it, the entry serves a page that links each platform's
  own page.
- The platform packages decide whether to add their keys when Expo starts
  reading keys, which is after every server entry has loaded, so the claim
  holds whatever the load order.
