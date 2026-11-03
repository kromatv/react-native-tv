# Expo CLI: config plugins and dev tools plugins

What `npx expo install` and `npx expo start` do with a package that ships a
config plugin and a dev tools plugin, as `@kromatv/react-native-tizen` does.
Checked on 2026-11-03.

| Tag | Meaning |
|---|---|
| **S** | Read in source: `@expo/cli` 58.1.18, `@expo/config-plugins` 58.0.11 and `expo-modules-autolinking` 58.0.16 as installed in this repository, under `build/`. A fresh `create-expo-app` (SDK 58) installed `@expo/cli` 58.1.19 and autolinking 58.0.17; the files compared below are unchanged there |
| **O** | Observed: a fresh `create-expo-app --template blank-typescript` app with this package installed from its packed tarball, on an arm64 Mac |

## 1. Config plugins

| Fact | Tag | Source |
|---|---|---|
| After installing, `expo install` runs `applyPluginsAsync` on the packages it installed. | S | `@expo/cli` `src/install/installAsync.js:202` |
| A package is added to `expo.plugins` when it has a config plugin in an `app.plugin.*` file, is not listed yet, and is not one of Expo's automatic plugins; it is added by name, without props. | S | `src/install/utils/autoAddConfigPlugins.js:48-56, 72-98` |
| A plugin is found as `<package>/app.plugin` with the extensions `.js .cjs .mjs .ts .cts .mts`, on disk whatever `exports` says; Expo warns when `exports` lacks `./app.plugin.js`. | S | `@expo/config-plugins` `build/utils/plugin-resolver.js:78-86, 100, 126` |
| An ESM plugin file loads: Expo requires it, or evaluates it itself when `require` of ESM is off. | S | `@expo/require-utils` `build/load.js:406-430` |
| Running Expo's `applyPluginsAsync` on the installed package printed `› Added config plugin: @kromatv/react-native-tizen` and wrote `"plugins": ["@kromatv/react-native-tizen"]` to `app.json`. | O | |
| `npx expo install <path to a .tgz>` installed the tarball under the key `"undefined"` in `package.json`, and added no plugin. | O | |
| A plugin that throws stops `npx expo config` with its message: a bad prop printed `Invalid @kromatv/react-native-tizen props:` and the field. | O | |

## 2. Dev tools plugins

| Fact | Tag | Source |
|---|---|---|
| Autolinking finds a dev tools plugin in a dependency whose `expo-module.config.json` lists `devtools` among its `platforms`, and reads `devtools.webpageRoot`, `bannerTitle`, `serverEntryPoint` and `cliExtensions`, each kept only when it is inside the package. | S | autolinking `build/ExpoModuleConfig.js:103-118`, `build/platforms/devtools.js:43-75`; `@expo/cli` `src/start/server/devtools/DevToolsPluginManager.js:63-68` |
| `cliExtensions` needs `description`, `commands` (at least one) and `entryPoint`; a command needs `name`, `title` and `environments` (`cli`, `mcp`), and may take `parameters` of type `text`, `number` or `confirm`. | S | `src/start/server/devtools/DevToolsPlugin.schema.js:23-98` |
| shift+m opens "Dev tools (native only)"; a plugin with a page and commands is one entry, which lists `Open <package>` and every command whose `environments` include `cli`. | S, O | `src/start/interface/startInterface.js:161`, `createDevToolsMenuItems.js:68-145` |
| Each parameter is asked with a prompt of its type, then "Execute command ..." is confirmed. An empty `text` answer is passed on as `""`. | S | `src/start/interface/cliExtensionMenuItemHandler.js:68-104` |
| The command runs as `node <entryPoint> <command> <args as JSON> <metroServerOrigin>`, with `process.execPath`, in the project root, with Expo's environment. Arguments must match the parameters in number and type. | S | `src/start/server/devtools/DevToolsPluginCliExtensionExecutor.js:56-79, 95-101` |
| `metroServerOrigin` is the dev server's `http` URL from Expo's URL creator: `http://192.168.1.124:8095` here, the LAN address. | S, O | `src/start/interface/interactiveActions.js:248`; `src/start/server/BundlerDevServer.js:374-381` |
| Expo kills the command with SIGKILL after 10 s (`DEFAULT_TIMEOUT_MS = 10_000`), and waits until its stdout and stderr close. | S | `DevToolsPluginCliExtensionExecutor.js:32, 128-145` |
| Output that parses as a JSON array of `{ type: 'text', text, level, url? }` (`level` one of `info`, `warning`, `error`) is shown as such, with `text: url` when a URL is given; any other output is shown line by line. | S, O | `DevToolsPluginCliExtensionResults.js:66-95`, `DevToolsPlugin.schema.js:130-195`, `cliExtensionMenuItemHandler.js:138-160` |
| A plugin's page and server live under `/_expo/plugins/<package>/`. The `serverEntryPoint` default-exports `handler(request)`, a Fetch API handler that sees the path without that prefix; returning nothing falls through to the files of `webpageRoot`. | S | `DevToolsPluginManager.js:36`, `middleware/DevToolsPluginMiddleware.js:65-123`, `DevToolsPluginServerHelpers.js:56-63` |
| The server's `webSocketHandlers`, if any, are mounted when Metro starts, at `/_expo/plugins/<package><route>`. | S | `src/start/server/metro/instantiateMetro.js:396-399`, `DevToolsPluginServerHelpers.js:64-83` |

## 3. Bundles for Tizen under `expo start`

| Fact | Tag | Source |
|---|---|---|
| `GET /.expo/.virtual-metro-entry.bundle?platform=tizen&dev=true&minify=false` on `expo start` returned a 4.7 MB development bundle for the example, through `withTizen`'s resolver. `/main.bundle` failed with `UnableToResolveError`, the server root being the workspace root, so the virtual entry, which Expo resolves to the app's `main`, is the URL to use. | O | |
| The simulator's runtime fetched that bundle from the Mac's dev server (`http://192.168.5.2:<port>`, Lima's `host.lima.internal`), then stopped at `TurboModuleRegistry.getEnforcing(...): 'DevSettings' could not be found`: a development bundle needs React Native's dev support modules, which the runtime does not serve yet. | O | simulator, 2026-11-03 |
| From a fresh app's `expo start`, shift+m, Run on TV built, signed and installed the development build on GQ75LS03DAUXZG (Tizen 9.0), whose runtime fetched the bundle from `http://192.168.1.124:8095` over the LAN and stopped at the same `DevSettings`. | O | TV, 2026-11-03 |

## 4. What Expo does not do for Tizen

| Fact | Tag | Source |
|---|---|---|
| `expo run:` exists for `ios` and `android` only. | S | `src/index.js:51-52` |
| `expo prebuild` generates `android` and `ios`; `all` means those two. | S | `src/prebuild/resolveOptions.js:201-219` |
| `expo export` accepts only the platforms Expo bundles with Metro and throws `Unsupported platform` for any other. `expo export:embed` takes a platform as given, which is why `rn-tizen bundle` uses it. | S | `src/export/resolveOptions.js:39-60`; `src/index.js:58` |
| The keys under `expo start` are Expo's own; a plugin adds commands only to the shift+m menu. | S | `src/start/interface/startInterface.js:150-175` |

## What this means for this package

- `app.plugin.js` re-exports the compiled config plugin. In a checkout of this
  repository, which has no `dist/`, it passes the config through, since Node
  22.13 cannot require the TypeScript source; `rn-tizen` checks the props
  whenever it reads them. Its props are the Tizen settings, validated with zod.
- `expo-module.config.json` declares the dev tools plugin. Each command starts
  a detached job and returns at once with the URL of the plugin's page, which
  shows each job's progress and output.
- A development build's runtime config carries `bundleUrl`, the virtual entry
  on the dev server Expo passes in, its host replaced when it is a loopback
  address the TV cannot reach.

## Sources

The files above, under `node_modules/.bun/@expo+cli@58.1.18*/node_modules/@expo/cli/build/`,
`node_modules/.bun/@expo+config-plugins@58.0.11*/`,
`node_modules/.bun/@expo+require-utils@*/` and
`node_modules/.bun/expo-modules-autolinking@58.0.16*/`.
