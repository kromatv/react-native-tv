# Expo CLI: config plugins and dev tools plugins

What `npx expo install` and `npx expo start` do with a package that ships a
config plugin and a dev tools plugin, as `@kromatv/react-native-tizen` does, and
how they load the app's Metro config. Checked on 2026-11-03 and 2026-11-04.

| Tag | Meaning |
|---|---|
| **S** | Read in source: `@expo/cli` 58.1.18, `@expo/config-plugins` 58.0.11, `@expo/metro-config` 58.0.20, `@expo/require-utils` 58.0.5, `metro-config` 0.87.1 and 0.87.2, and `expo-modules-autolinking` 58.0.16 as installed in this repository, under `build/` (`src/` for `metro-config`). A fresh `create-expo-app` (SDK 58) installed `@expo/cli` 58.1.19 and autolinking 58.0.17; the files compared below are unchanged there |
| **O** | Observed: a fresh `create-expo-app --template blank-typescript` app with this package installed from its packed tarball, on an arm64 Mac |
| **P** | Observed in section 6: an app in `/tmp` with `expo` ~58.0.16 (58.0.23 installed, `@expo/cli` 58.1.19), `expo-image`, `react-native-svg` 15.16.0 and `react` 19.3.0, installed on an arm64 Mac on 2026-11-04 with npm 11.17.0, Bun 1.4.2, pnpm 9.15.9, 10.34.9, 11.28.9 and 12.16.1, and Yarn 1.22.22 and 4.18.1 (`nodeLinker: node-modules`). `fake-lib`, a local tarball that depends on `react-native` 0.88.0, tested the overrides |
| **D** | Read in the documentation linked at the end |

## 1. Config plugins

| Fact | Tag | Source |
|---|---|---|
| After installing, `expo install` runs `applyPluginsAsync` on the packages it installed. | S | `@expo/cli` `src/install/installAsync.js:202` |
| A package is added to `expo.plugins` when it has a config plugin in an `app.plugin.*` file, is not listed yet, and is not one of Expo's automatic plugins; it is added by name, without props. | S | `src/install/utils/autoAddConfigPlugins.js:48-56, 72-98` |
| A plugin is found as `<package>/app.plugin` with the extensions `.js .cjs .mjs .ts .cts .mts`, on disk whatever `exports` says; Expo warns when `exports` lacks `./app.plugin.js`. | S | `@expo/config-plugins` `build/utils/plugin-resolver.js:78-86, 100, 126` |
| An ESM plugin file loads: Expo requires it, or evaluates it itself when `require` of ESM is off. | S | `@expo/require-utils` `build/load.js:406-430` |
| Expo takes the module's `default` export when it has one, then requires a function. | S | `build/utils/plugin-resolver.js:204-208` |
| A plugin listed by a path (starting with `.`, `/` or `~/`) resolves from the project root with the same extensions. | S | `build/utils/plugin-resolver.js:88-97, 137-142` |
| For a TypeScript plugin file, Expo transpiles or strips the types of that file only; the `.ts` files it imports are loaded by Node. On Node 22.13.1 the example's plugin listed as `../src/config/config-plugin.ts` failed with `SyntaxError: Unexpected identifier 'PluginMatcher'`, a type in an imported file; on Node 26.5 it loaded. | S, O | `@expo/require-utils` `build/load.js:333-360` |
| Running Expo's `applyPluginsAsync` on the installed package printed `› Added config plugin: @kromatv/react-native-tizen` and wrote `"plugins": ["@kromatv/react-native-tizen"]` to `app.json`. | O | |
| `npx expo install <path to a .tgz>` installed the tarball under the key `"undefined"` in `package.json`, and added no plugin. | O | |
| In a Bun workspace, `npx rn-tizen` answered 404 from the npm registry while `bunx rn-tizen` ran the package's command (KROMA's acceptance test, Bun 1.4.2, 2026-11-04). | O | |
| A plugin that throws stops `npx expo config` with its message: a bad prop printed `Invalid @kromatv/react-native-tizen props:` and the field. | O | |

## 2. Dev tools plugins

| Fact | Tag | Source |
|---|---|---|
| Autolinking finds a dev tools plugin in a dependency whose `expo-module.config.json` lists `devtools` among its `platforms`, and reads `devtools.webpageRoot`, `bannerTitle`, `serverEntryPoint` and `cliExtensions`, each kept only when it is inside the package. | S | autolinking `build/ExpoModuleConfig.js:103-118`, `build/platforms/devtools.js:43-75`; `@expo/cli` `src/start/server/devtools/DevToolsPluginManager.js:63-68` |
| Autolinking walks the app's `dependencies`, `devDependencies` and required `peerDependencies`, not every folder in `node_modules`. This repository's example names no dependency on the package, so shift+m lists no Tizen entry there until the package is named in its `package.json` and linked into its `node_modules`. | S, O | autolinking `build/dependencies/resolution.js:51-66`; example, 2026-11-03 |
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
| The simulator's runtime fetched that bundle from the Mac's dev server (`http://192.168.5.2:<port>`, Lima's `host.lima.internal`), then stopped at `TurboModuleRegistry.getEnforcing(...): 'DevSettings' could not be found`: a development bundle needs React Native's dev support modules, which the runtime did not serve then. | O | simulator, 2026-11-03 |
| From a fresh app's `expo start`, shift+m, Run on TV built, signed and installed the development build on GQ75LS03DAUXZG (Tizen 9.0), whose runtime fetched the bundle from `http://192.168.1.124:8095` over the LAN and stopped at the same `DevSettings`. | O | TV, 2026-11-03 |
| With the development runtime serving them ([react-native-dev-tooling.md](react-native-dev-tooling.md)), shift+m, Run in simulator in the example's `expo start` loaded the bundle from `http://192.168.1.124:8090`, joined `/message`, printed the app's `console.log` in Expo's terminal, and an edit reached the screen through Fast Refresh with focus and state kept. | O | simulator, 2026-11-03 |
| `rn-tizen run --dev-server http://localhost:8090` on GQ75LS03DAUXZG (The Frame, 2024, Tizen 9.0) wrote `http://192.168.1.124:8090/...` into `bundleUrl`; the app joined `/message`, and an edit re-ran its effects without running `main` again. | O | TV, 2026-11-03 |

## 4. What Expo does not do for Tizen

| Fact | Tag | Source |
|---|---|---|
| `expo run:` exists for `ios` and `android` only. | S | `src/index.js:51-52` |
| `expo prebuild` generates `android` and `ios`; `all` means those two. | S | `src/prebuild/resolveOptions.js:201-219` |
| `expo export` accepts only the platforms Expo bundles with Metro and throws `Unsupported platform` for any other. `expo export:embed` takes a platform as given, which is why `rn-tizen bundle` uses it. | S | `src/export/resolveOptions.js:39-60`; `src/index.js:58` |
| The keys under `expo start` are Expo's own; a plugin adds commands only to the shift+m menu. | S | `src/start/interface/startInterface.js:150-175` |

## 5. The Metro config

| Fact | Tag | Source |
|---|---|---|
| `expo start` and `expo export:embed` load the app's Metro config with `loadUserConfig` from `@expo/metro-config`, not with Metro's own `loadConfig`, and merge it over Expo's default config. | S | `@expo/cli` `src/start/server/metro/instantiateMetro.js:241-245`; `@expo/metro-config` `build/config/loadUserConfig.js:24-27` |
| `loadUserConfig` looks for `metro.config`, then `.config/metro`, with `.js .cjs .mjs .json .ts .cts .mts`, from the project root up to the Metro server root, then for a `metro` key in the project's `package.json`. It loads the file with `loadModuleSync` and takes `default` when the module sets `__esModule`. | S | `@expo/metro-config` `build/config/resolveMetroUserConfig.js:48-56, 83-91, 100-124` |
| `loadModuleSync` compiles a `.mjs` file as an ES module itself (`Module._compile` with the `module` format), so `import.meta.dirname` and named imports work in it. | S | `@expo/require-utils` `build/load.js:135-157, 205-265, 406-431` |
| Metro's own `loadConfig` searches the same names and falls back to `import()` when `require` fails. Expo 58 ships `metro-config` 0.87.1 through `@expo/metro` 58.0.0. | S | `metro-config` 0.87.1 `src/loadConfig.js:57-64, 303-323`; 0.87.2 `src/loadConfig.js:57-64, 328-348` |
| `withMetroMultiPlatformAsync` sets `resolver.platforms` to the platforms of the Expo config plus those of the Metro config. No step lets a package add a platform, and config plugins change only the app config and the native projects, so `withTizen` must wrap the Metro config. | S | `src/start/server/metro/withMetroMultiPlatform.js:719-765` |
| `expo/metro-config` is `module.exports = require('@expo/metro-config')`, and `import { getDefaultConfig } from 'expo/metro-config'` gives the function on Node 22.13.1 and 26.5. | S, O | `expo` 58.0.22 `metro-config.js:1` |
| `loadUserConfig({ projectRoot, serverRoot: getMetroServerRoot(projectRoot) })`, from `expo/metro-config` and `@expo/config/paths`, returned the example's config in 0.6 s, its `resolver.platforms` `ios android tvos macos tizen`. | S, O | `@expo/config` 58.0.10 `build/paths/paths.js:31-32` |
| `expo prebuild` suggests an Android package from the iOS bundle id when it is valid, else `com.<owner>.<slug>` with `anonymous` for an owner it does not know. It sanitizes it: characters other than letters, digits, `_` and `.` dropped, empty segments removed, `com.` added until there are two segments, and `x` before a segment that does not start with a letter. | S | `src/utils/getOrPromptApplicationId.js:93, 192-210`; `src/utils/validateApplicationId.js:108-130` |

## 6. react-native-tvos and the package managers

| Fact | Tag | Source |
|---|---|---|
| "You cannot use this package and the core react-native package simultaneously in a project." An app installs it as `"react-native": "npm:react-native-tvos@<version>"`. | D | [rntvos] |
| Expo's TV guide: "In **package.json**, modify the `react-native` dependency to use the TV repo", `npm:react-native-tvos@0.85-stable` for SDK 56 and later, and "The `react-native-tvos` version must match the Expo SDK you are using." | D | [expo-tv] |
| Expo treats an app as a TV app when its installed `react-native` is named `react-native-tvos`. `expo install --check` then compares only the major and minor version with Expo's bundled `react-native`, 0.88.1 in SDK 58, and suggests `npm:react-native-tvos@<major>.<minor>-stable` when that dist-tag is published, `@next` for a prerelease, else `@latest`. | S | `@expo/cli` 58.1.19 `build/src/start/doctor/dependencies/reactNativeTv.js:70-130`, `validateDependenciesVersions.js:242-266`; `expo` 58.0.23 `bundledNativeModules.json:105` |
| react-native-tvos 0.88.0-0 depends on `@react-native-tvos/virtualized-lists` 0.88.0-0, whose peer is `"react-native": "0.88.0"`. No semver range, `*` included, matches the prerelease `0.88.0-0`. With the alias alone npm installs upstream `react-native` 0.88.0 for that peer at `node_modules/react-native/node_modules/react-native`, 31 MB. Bun, every pnpm and both Yarns install one copy. | P | `@react-native-tvos/virtualized-lists/package.json` |
| npm: "overrides may also be defined as a reference to a spec for a direct dependency" "by prefixing the name of the package you wish the version to match with a `$`". With `"overrides": { "react-native": "$react-native" }`, a fresh npm install has one copy, and so does switching an installed app from `react-native` 0.88.1 to the alias and the override in one `npm install`. | D, P | [npm-overrides] |
| Added to an app whose `package-lock.json` holds the second copy, the override changes nothing: `npm install` and `npm dedupe` answer `up to date`. The lock's entry for upstream `react-native` has no `name`; the alias's has `"name": "react-native-tvos"`. Deleting the upstream entries from the lock, then `npm install`, removed the copy (`removed 2 packages`). | P | |
| Without an override, every manager gives `fake-lib` its own upstream `react-native`. With one, `fake-lib` loads react-native-tvos: npm and Bun with `"overrides": { "react-native": "$react-native" }`, pnpm 10.34.9 with the same under `pnpm.overrides` in `package.json`, pnpm 11.28.9 and 12.16.1 with `overrides` in `pnpm-workspace.yaml`, and both Yarns with `"resolutions": { "react-native": "npm:react-native-tvos@0.88.0-0" }`. | P | |
| Bun: "A value of `"$name"` reuses the range you declared for `name` in your own dependencies", and "Bun only reads overrides from the root `package.json`". | D | [bun-overrides] |
| Yarn's `resolutions` "can only be set at the root of the project"; its documentation shows versions as values and no `$` reference. | D | [yarn-resolutions] |
| pnpm: "pnpm does not read settings from the `pnpm` field of `package.json`. Define settings in `pnpm-workspace.yaml` instead." pnpm 12.16.1 printed `The "pnpm" field in package.json is no longer read by pnpm. The following keys were ignored: "pnpm.overrides"`. pnpm 9.15.9 refused a `pnpm-workspace.yaml` without `packages`: `packages field missing or empty`. | D, P | [pnpm-package-json], [pnpm-overrides] |
| Added to an existing install, the override replaced `fake-lib`'s copy with Bun, pnpm 12.16.1 and both Yarns. pnpm 10.34.9 wrote it into `pnpm-lock.yaml` and left `fake-lib` linked to the upstream copy. | P | |
| Bun 1.4.2 fails on a dist-tag in an alias: `No version matching "npm:react-native-tvos@0.87-stable" found for specifier "react-native-tvos" (but package exists)`. | P | |
| Five hours after react-native-tvos 0.88.1-0 was published, Yarn 4.18.1 refused it (`All versions satisfying "0.88.1-0" are quarantined`) and pnpm 12.16.1 resolved `0.88-stable` to `0.88.0-0rc2`. Semver sorts `0.88.0-0rc2` after `0.88.0-0`, so `~0.88.0-0` resolves to the release candidate. | P | `semver` 7 `compare`, `maxSatisfying` |

## 7. Keys in `expo start`

| Fact | Tag | Source |
|---|---|---|
| Expo's documentation describes no `devtools` key but `devtools` as a value of `platforms`: not `webpageRoot`, `serverEntryPoint` or `cliExtensions`, not the arguments a command receives, no way for plugin code to learn the dev server's address, and no plugin keys in the terminal UI. | D | [module-config], [create-devtools-plugins], [devtools-plugins] |
| A plugin's `serverEntryPoint` is loaded when the dev server starts, to read its `webSocketHandlers`, before the terminal UI starts, and through Node's `require` or `import`, so it runs once per process. | S | `src/start/server/metro/instantiateMetro.js:389-404`, `src/start/startAsync.js:175-197`; `@expo/require-utils` `build/load.js:393-431` |
| A request under `/_expo/plugins/<package>/` goes to the entry's default-exported handler first; when it returns nothing, Expo serves the file from `webpageRoot`. | S | `src/start/server/middleware/DevToolsPluginMiddleware.js:76-87, 92-103` |
| The terminal UI runs only when interactive: not in CI and with stdout a TTY. | S | `src/utils/interactive.js`, `src/start/startAsync.js:189-197` |
| The UI prints the server's address and the usage, then creates its `KeyPressHandler`, which reads keys with `stdin.on('data', this.handleKeypress)` in raw mode, UTF-8, one chunk a key. | S | `src/start/interface/startInterface.js:105, 273-277`, `src/start/interface/KeyPressHandler.js:72-117` |
| While one of Expo's prompts is open (`pauseInteractions`), the handler takes its listener off stdin and leaves raw mode, and puts it back after. | S | `src/start/interface/KeyPressHandler.js:59-75, 118-131`, `src/utils/prompts.js:121-143` |
| A key with no case in `onPressAsync` does nothing: `t` and `T` reach no Expo action. `?` prints the full list synchronously, inside the handler. | S | `src/start/interface/startInterface.js:106-153, 158-268` |
| A usage line is `› Press <key bold> <│ dim> <text>`, and `› shift+a │ ...` for a shifted key; the full list adds `shift+a` (select an Android device or emulator) and `shift+i`. `shift+a` and `shift+i` ask for a device first. | S | `src/start/interface/commandsTable.js:100-116, 145-190, 255-270`; `src/start/interface/startInterface.js:170-205` |
| Right after Metro starts, Expo sets `EXPO_DEV_SERVER_ORIGIN` to `http://localhost:<port>` (`https` with `--https`) in its own process. expo-modules-core declares it in its published `ExpoProcessEnv` type ("Used in `@expo/metro-runtime`"). Expo also sets `RCT_METRO_PORT` to the port it picked. | S | `src/start/server/metro/MetroBundlerDevServer.js:977-978`; `expo-modules-core` `build/ts-declarations/global.d.ts:13-14`; `src/utils/port.js:224-226` |
| In a standalone npm app (`/tmp`, packed tarball): `npx expo start` printed `› Press t │ open on Tizen TV` after `› Press ? │ show all commands`; `?` printed both lines after Expo's list; `t` and `T` typed into the open shift+m menu opened nothing; shift+t listed the TV used last, named from the scan, and the simulator; `2` opened the app in the simulator from the dev server, with Expo bundling `index.js` for `tizen` in its own terminal; an edit of a component module kept the app's state, an edit of the entry file reloaded it; `r` reloaded it; the plugin's page and `/api/jobs` still answered. | O | simulator, 2026-11-04 |
| In the same app, `t` opened the app on GQ75LS03DAUXZG (The Frame, 2024, Tizen 9.0), the TV used last: Expo's terminal showed the build, install and launch, then the TV's `console.log`; an edit that renamed a log line went from `tick 20` to `tock 25`, the new code with the state kept; `r` restarted the count at 0. | O | TV, 2026-11-04 |
| In a standalone Bun app (Bun 1.4.2, no config file), `bunx expo start` printed the same line; `t` and `T` typed into the shift+m menu opened nothing; shift+t, then `2`, opened the app in the simulator; an edit kept its state and `r` reloaded it. | O | simulator, 2026-11-04 |
| `simulator run --dev-server` wrote `tizen/build/bundle` without claiming `tizen/`, so the next TV build refused it as the app's own. It claims the directory first now. | O | simulator, 2026-11-04 |

## What this means for this package

- `app.plugin.js` re-exports the compiled config plugin as an ES module, plain
  JavaScript that Node loads under `node_modules`, where it refuses to strip
  types. Its props are the Tizen settings, validated with zod. The example in
  this repository lists `../app.plugin.js`: listing the TypeScript source fails
  on Node 22.13 (section 1), and `bun install` builds `dist/` through the
  `prepare` script.
- `rn-tizen init` writes `metro.config.mjs` or wraps an existing config, and
  suggests the app's `android.package`, else `com.anonymous.<slug>` sanitized
  as `expo prebuild` does, for the package id. Builds load the Metro config
  with `loadUserConfig` before anything else, and stop with `npx rn-tizen init`
  as the fix when its platforms lack `tizen`.
- `rn-tizen init` points `react-native` at `npm:react-native-tvos@0.88.0-0`,
  the release this repository builds against, unless the app names a
  react-native-tvos already. An exact version, because Bun refuses the
  `-stable` tag and a range can pick a release candidate; Expo accepts it, as
  it compares 0.88 only. It writes the override where the manager of the
  nearest lockfile reads it (section 6): `$react-native` where the manager
  takes it and the root depends on `react-native`, else the spec. For npm it
  drops the lock's upstream entries, then it runs the install and checks that
  one copy is left. Builds, shift+m and `doctor` follow the app's dependencies,
  optional dependencies and peers from where each package is installed, as
  Node looks them up, and stop when they reach more than one `react-native`.
- `expo-module.config.json` declares the dev tools plugin. Each command starts
  a detached job and returns at once with the URL of the plugin's page, which
  shows each job's progress and output.
- The plugin's server entry adds `t` and shift+t to an interactive `expo
  start` (section 7): once Expo's `handleKeypress` listener is on stdin, it
  prints its usage line and listens after it, and acts only while that
  listener is there and stdin is raw, so a key typed into an Expo prompt does
  nothing. shift+t takes Expo's listener off while its list is open, as Expo
  does for its own prompts. Opening runs Run on TV's job with `--no-logs`
  against `EXPO_DEV_SERVER_ORIGIN`, the only address Expo gives code in its
  process, and copies the job's output to Expo's terminal; the app's logs
  reach that terminal through Metro.
- A development build's runtime config carries `bundleUrl`, the virtual entry
  on the dev server Expo passes in, its host replaced when it is a loopback
  address the TV cannot reach.

## Sources

The files above, under `node_modules/.bun/@expo+cli@58.1.18*/node_modules/@expo/cli/build/`,
`node_modules/.bun/@expo+config-plugins@58.0.11*/`,
`node_modules/.bun/@expo+require-utils@*/`,
`node_modules/.bun/@expo+metro-config@58.0.20*/`,
`node_modules/.bun/@expo+config@58.0.10*/`,
`node_modules/.bun/metro-config@0.87.1*/`, `node_modules/.bun/metro-config@0.87.2*/`,
`node_modules/.bun/expo@58.0.22*/` (with its `expo-modules-core`) and
`node_modules/.bun/expo-modules-autolinking@58.0.16*/`.

[module-config]: https://docs.expo.dev/modules/module-config/
[create-devtools-plugins]: https://docs.expo.dev/debugging/create-devtools-plugins/
[devtools-plugins]: https://docs.expo.dev/debugging/devtools-plugins/
[rntvos]: https://github.com/react-native-tvos/react-native-tvos
[expo-tv]: https://docs.expo.dev/guides/building-for-tv/
[npm-overrides]: https://docs.npmjs.com/cli/v11/configuring-npm/package-json#overrides
[bun-overrides]: https://bun.sh/docs/install/overrides
[yarn-resolutions]: https://yarnpkg.com/configuration/manifest#resolutions
[pnpm-package-json]: https://pnpm.io/package_json
[pnpm-overrides]: https://pnpm.io/settings/dependency-resolution#overrides
