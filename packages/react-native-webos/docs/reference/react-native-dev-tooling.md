# React Native dev tooling

What an app in development needs from its host so that `npx expo start` drives
it as it drives an Android device: Fast Refresh, `r` to reload, logs in the
terminal, LogBox, and `j` for React Native DevTools. Read from React Native's
Android host, Metro and Expo's CLI.
The host side lives in `runtime/host/devsupport/` and is compiled into the
development runtime only ([decisions.md](../decisions.md)).

## How to read this page

| Tag | Meaning |
|-----|---------|
| **doc** | Read in the cited source line. |
| **inf** | Inferred; the reason is given next to it. |
| **sim** | Verified in the simulator, with the date. |

Line numbers are pinned to these builds, read on 2026-11-03:

| Prefix | Path under `node_modules/.bun/` |
|--------|------|
| `RN/` | `react-native-tvos@0.88.0-0+c1077d8cda41e7c4/node_modules/react-native-tvos/` |
| `RA/` | `RN/ReactAndroid/src/main/java/com/facebook/react/` |
| `RC/` | `RN/ReactCommon/react/` |
| `METRO/` | `metro@0.87.2+759ce506b1ed1a42/node_modules/metro/src/` |
| `CLI/` | `@expo+cli@58.1.18+ff75e6402ef08474/node_modules/@expo/cli/build/src/` |
| `EXPO/` | `expo@58.0.22+a11af42e038f0000/node_modules/` |

## 1. The dev server and the bundle URL

| Fact | Source |
|------|--------|
| Android asks for `<server>/<module>.bundle?platform=android&dev=true&lazy=true&minify=false&app=<package>&modulesOnly=false&runModule=true` | doc: `RA/devsupport/DevServerHelper.kt:265-298` |
| Expo's Android host names the module `.expo/.virtual-metro-entry`; Expo's Metro rewrites that path to the app's entry for whatever `platform` the URL names, and adds `transform.engine=hermes` only for `android` and `ios` | doc: `EXPO/expo/android/src/main/java/expo/modules/ExpoReactHostFactory.kt:100`, `EXPO/@expo/metro-config/build/rewriteRequestUrl.js:126-185` |
| JavaScript finds the dev server from `SourceCode.scriptURL`: the URL's origin is the server, the whole URL the bundle HMR registers | doc: `RN/Libraries/Core/Devtools/getDevServer.js:28-40` |
| `npx expo start` serves a `platform=tizen` bundle through `withTizen` | sim, 2026-11-03: `tizen Bundled 1876ms example/main.ts (736 modules)` |

So a Tizen build needs one URL: rn-tizen writes
`http://<host>:<port>/.expo/.virtual-metro-entry.bundle?platform=tizen&dev=true&lazy=true&minify=false&app=<package id>&modulesOnly=false&runModule=true&excludeSource=true&sourcePaths=url-server&transform.engine=hermes&unstable_transformProfile=hermes-stable` (the `excludeSource` pair for DevTools, section 9)
into `bundleUrl` (`src/package/dev-server-bundle.ts`), `<host>:<port>` being the
`--dev-server` origin with a loopback host replaced by one the TV reaches, and
the host derives every other endpoint from it (`devsupport/DevServer.cpp`).

## 2. The packager socket: `r` and `m`

| Fact | Source |
|------|--------|
| The app opens `ws://<server>/message?device=<name>&app=<package>&clientid=<id>` | doc: `RA/packagerconnection/JSPackagerClient.kt:30-41` |
| Messages are JSON with `version` 2, a `method`, and an `id` when an answer is wanted; a message of another version is dropped, an unknown method with an `id` is answered with an `error` | doc: `JSPackagerClient.kt:52-95,129` |
| Android handles `reload` (reload the JavaScript) and `devMenu` (show the developer menu) | doc: `RA/devsupport/DevServerHelper.kt:149-160`, `RA/devsupport/DevSupportManagerBase.kt:979-989` |
| While the server is away the socket tries again every 2 s | doc: `RA/packagerconnection/ReconnectingWebSocket.kt:62-71,148` |
| `r` in `expo start` broadcasts `{version: 2, method: "reload"}` to every client of `/message` | doc: `CLI/start/interface/interactiveActions.js:222`, `CLI/start/server/metro/dev-server/createMessageSocket.js:72-82` |

Host: `devsupport/PackagerConnection.cpp`. sim, 2026-11-03: `r` printed
"Reloading apps" and the app fetched the bundle again in the same process; the
socket reconnected within 2 s of the server restarting. Tizen has no developer
menu yet: `devMenu` logs a line pointing at `r` and `j`.

## 3. HMR and Fast Refresh

| Fact | Source |
|------|--------|
| Once the instance exists, Android calls `HMRClient.setup("android", <bundle path without its leading slash>, host, port, isHotModuleReplacementEnabled, scheme)`; the call is buffered until the bundle has run | doc: `RA/devsupport/DevSupportManagerBase.kt:699-716`, `RC/runtime/ReactInstance.cpp:345-395` |
| `HMRClient` is a callable module only in a `__DEV__` bundle | doc: `RN/Libraries/Core/setUpBatchedBridge.js:33-40` |
| `setup` opens `<server>/hot`, registers the full bundle URL as an entry point and turns Fast Refresh on when `isEnabled` | doc: `RN/Libraries/Utilities/HMRClient.js:152-295,323-338` |
| Metro rewrites a registered URL with the server's `rewriteRequestUrl`, so Expo's virtual entry works there too | doc: `METRO/HmrServer.js:92` |
| Expo replaces React Native's `HMRClient.js` with `expo/src/async-require/hmr.ts`, same `setup` signature, when the module resolves under `react-native/`; a react-native installed under another folder name keeps React Native's | doc: `CLI/start/server/metro/withMetroMultiPlatform.js:598`, `EXPO/expo/src/async-require/hmr.ts:152-269` |
| Turning Fast Refresh on or off from `DevSettings.setHotLoadingEnabled` stores the setting and reloads | doc: `RA/devsupport/DevSupportManagerBase.kt:897-905` |

sim, 2026-11-03: an edit to `example/src/app.tsx` changed the title on screen
while the selected card, the uptime counter and the last remote key kept their
values.

## 4. Logs in the dev server's terminal

| Fact | Source |
|------|--------|
| In a `__DEV__` bundle whose console is React Native's polyfill, every `console` method also calls `HMRClient.log(level, args)` | doc: `RN/Libraries/Core/setUpDeveloperTools.js:19-46` |
| The polyfill is installed when the host provides `nativeLoggingHook`, and marks itself `_isPolyfilled` | doc: `node_modules/.bun/@react-native+js-polyfills@0.88.1/node_modules/@react-native/js-polyfills/console.js:579,651` |
| `HMRClient.log` sends `{type: "log", level, data}` over the `/hot` socket; it keeps the last 100 lines logged before `setup` and sends them once connected | doc: `RN/Libraries/Utilities/HMRClient.js:116-148,340-348` |
| Metro reports it as a `client_log` event, `server.forwardClientLogs` being true by default | doc: `METRO/HmrServer.js:209-218`, `node_modules/.bun/metro-config@0.87.2+759ce506b1ed1a42/node_modules/metro-config/src/defaults/index.js:92` |
| Expo's terminal prints a `client_log` as ` LOG `, ` WARN ` or ` ERROR ` and the message, symbolicating a stack it finds | doc: `CLI/start/server/metro/MetroTerminalReporter.js:136-146,268-330` |

So the host needs nothing beyond `bindNativeLogger` and the HMR setup. sim,
2026-11-03: `console.log` and `console.warn` from the TV printed in `expo
start`'s terminal as they do for an Android device. Native log lines and
native crashes never reach Metro: they travel over rn-tizen's log stream only.

## 5. DevSettings

| Fact | Source |
|------|--------|
| `DevSettings` is `getEnforcing`: a `__DEV__` bundle dies at startup without it, since `HMRClient` imports it | doc: `RN/src/private/specs_DEPRECATED/modules/NativeDevSettings.js:15-33`, `RN/Libraries/Utilities/HMRClient.js:17` |
| `DevSettings.reload(reason)` calls `reloadWithReason` when the module has it | doc: `RN/Libraries/Utilities/DevSettings.js:80-86` |
| `toggleElementInspector` makes Android emit the `toggleElementInspector` device event | doc: `RA/runtime/ReactHostImplDevHelper.kt:53-58` |

Host: `devsupport/DevSettingsModule.cpp`. sim, 2026-11-03:
`DevSettings.reload('Select pressed')` reloaded the app.

## 6. LogBox

| Fact | Source |
|------|--------|
| `AppRegistry` registers the inspector as the app `LogBox` | doc: `RN/Libraries/ReactNative/AppRegistry.js:14-18` |
| JavaScript calls `NativeLogBox.show()` when a log is selected and `hide()` when none is; the module is optional | doc: `RN/Libraries/LogBox/Data/LogBoxData.js:314-321`, `RN/src/private/specs_DEPRECATED/modules/NativeLogBox.js:15-20` |
| Android runs the `LogBox` app in a surface of its own, in a dialog over the activity | doc: `RA/devsupport/LogBoxDialogSurfaceDelegate.kt:22-40`, `RA/devsupport/LogBoxModule.kt:26-45` |
| Android numbers root views 1, 11, 21: React's own tags are even, so they never meet | doc: `RA/uimanager/ReactRootViewTagGenerator.kt:15-21` |
| In a `__DEV__` bundle an exception goes to LogBox alone, never to `ExceptionsManager`: without LogBox, errors are invisible on the device | doc: `RN/Libraries/Core/ExceptionsManager.js:111-134` |
| The yellow box of warnings is part of the app's own tree; react-native-tvos draws it at the top on a TV | doc: `RN/Libraries/LogBox/LogBoxNotificationContainer.js:84`, `RN/Libraries/LogBox/UI/LogBoxNotification.js:55-83` |
| Back (`hardwareBackPress`) minimizes the inspector, and clears the yellow box once it has focus | doc: `RN/Libraries/LogBox/LogBoxInspectorContainer.js:29-41`, `LogBoxNotificationContainer.js:27-41` |

Stopping a surface commits an empty tree, which mounts synchronously, then
destroys the shadow tree, whose destructor revokes the mounting coordinator
(doc: `RC/renderer/scheduler/SurfaceHandler.cpp:79-98`,
`RC/renderer/mounting/ShadowTree.cpp:263-265,622-635`,
`RC/renderer/scheduler/Scheduler.cpp:291-313`). iOS mounts a commit made on
the main thread inline (doc: `RN/React/Fabric/Mounting/RCTMountingManager.mm:187-202`);
Tizen's `SurfacePresenter` queues every mount, so it mounts the stopping
overlay's last transaction inline or its views would outlive their root.

Host: `devsupport/LogBoxModule.cpp`, `SurfacePresenter::startOverlaySurface`.
sim, 2026-11-03: a thrown render error showed the red inspector, symbolicated
to `app.tsx:70:42`, and printed in `expo start`'s terminal; Back minimized it;
fixing the code brought the app back. A `console.warn` showed the yellow box.

Open: the inspector's buttons need focus, which Tizen does not move between
views yet, so Back is the only way to dismiss it; and a `Modal` open when an
error arrives draws above the inspector, since modals sit on a higher Evas
layer than any surface.

## 7. The loading banner

| Fact | Source |
|------|--------|
| `DevLoadingView.showMessage(message, textColor, backgroundColor, dismissButton)` and `hide()`, optional | doc: `RN/src/private/specs_DEPRECATED/modules/NativeDevLoadingView.js:15-25` |
| The colors come from `processColor`: `load` is white on `#404040`, `refresh` white on `#2584e8` | doc: `RN/Libraries/Utilities/DevLoadingView.js:14-80` |
| Android shows "Loading from host:port…" while it downloads a bundle | doc: `RA/devsupport/DevSupportManagerBase.kt:730-747`, `RN/ReactAndroid/src/main/res/devsupport/values/strings.xml:35` |
| HMR shows "Refreshing..." on `update-start` and hides it on `update-done` | doc: `RN/Libraries/Utilities/HMRClient.js:216-243` |

Host: `devsupport/LoadingBanner.cpp`, `devsupport/DevLoadingViewModule.cpp`.

## 8. Reload and teardown

| Fact | Source |
|------|--------|
| Reload is a new instance: Android and iOS destroy the instance and start another on the same root view | doc: `RA/devsupport/DevSupportManagerBase.kt:979-985`, `RN/ReactCommon/react/runtime/platform/ios/ReactCommon/RCTInstance.mm:222-250` |
| `TimerCallback` holds the timer's `jsi::Function`. iOS gives the `TimerManager` to the instance alone, whose members free it before the runtime | doc: `RC/runtime/TimerManager.h:27-40`, `RN/ReactCommon/react/runtime/platform/ios/ReactCommon/RCTInstance.mm:311-333`, `RC/runtime/ReactInstance.h:87-97` |

The Tizen host kept its own reference to the `TimerManager`, so on reload the
old one died after its runtime and freed functions of a dead heap (sim,
2026-11-03: SIGSEGV in `~unordered_map<int, TimerCallback>` on the second
reload). It now drops that reference on the JavaScript thread before the
instance.

## 9. React Native DevTools (`j`)

| Fact | Source |
|------|--------|
| `j` in `expo start` lists `/json/list`, keeps the pages whose capabilities include `nativePageReloads`, and posts `/open-debugger?target=<page>` for the one picked; a page without `reactNative.logicalDeviceId` is refused | doc: `CLI/start/interface/interactiveActions.js:198-216`, `CLI/start/server/middleware/inspector/JsInspector.js:46-71`, `CLI/start/server/metro/debugging/pageIsSupported.js:11-14` |
| The proxy takes a device on `/inspector/device?name=<name>&app=<app>&device=<id>&profiling=<bool>`, identifies it by `device`, and lists each page with that id as `logicalDeviceId` | doc: `node_modules/.bun/@react-native+dev-middleware@0.88.1+759ce506b1ed1a42/node_modules/@react-native/dev-middleware/dist/inspector-proxy/InspectorProxy.js:188-205,236-275` |
| Android's device id is a hash of the package, the device's ID and `fusebox`, stable across launches | doc: `RA/devsupport/DevServerHelper.kt:94-131` |
| The device side of that protocol is jsinspector-modern's `InspectorPackagerConnection`; the platform gives it WebSockets and delayed callbacks, all on the inspector queue, and it reconnects with a growing delay | doc: `RN/ReactCommon/jsinspector-modern/InspectorPackagerConnection.h:30-82`, `RN/ReactCommon/jsinspector-modern/WebSocketInterfaces.h:35-70`, `InspectorPackagerConnection.cpp:24-26,370-385` |
| The host creates a `HostTarget` and lists it as the page "React Native Bridgeless" with `nativePageReloads`; it removes the page before destroying the target. `Page.reload` and the "Paused in debugger" overlay come back through the `HostTargetDelegate` | doc: `RN/ReactAndroid/src/main/jni/react/runtime/jni/JReactHostInspectorTarget.cpp:35-63,86-124`, `RN/ReactCommon/jsinspector-modern/HostTarget.h:164-241` |
| iOS's `HostTarget` executor runs a callback inline on the main queue | doc: `RN/ReactCommon/react/runtime/platform/ios/ReactCommon/RCTHost.mm:392-396`, `RN/React/Base/RCTUtils.mm:289-298` |
| Each `ReactInstance` given the target registers itself and its runtime through that executor, holding JavaScript work until it has; it must unregister on the main thread before it is destroyed | doc: `RC/runtime/ReactInstance.cpp:112-166,202-212`, `RA/runtime/ReactHostImpl.kt:1637-1648` |
| Android's paused overlay resumes the runtime with `HostCommand::DebuggerResume`, a `Debugger.resume` sent over a private session | doc: `RA/runtime/ReactHostImpl.kt:420-435`, `RN/ReactCommon/jsinspector-modern/HostTarget.cpp:128-157,282-289` |
| jsinspector-modern turns Fusebox on when compiled with `REACT_NATIVE_DEBUGGER_ENABLED`; React Native's CMake defines it, and `HERMES_ENABLE_DEBUGGER` for the Hermes glue, in a Debug build or with `REACT_NATIVE_DEBUG_OPTIMIZED`, which Android's `debugOptimized` variant passes | doc: `RN/ReactCommon/jsinspector-modern/InspectorFlags.cpp:68-73`, `RN/ReactCommon/jsinspector-modern/CMakeLists.txt`, `RN/ReactCommon/react/runtime/hermes/CMakeLists.txt`, `RN/ReactAndroid/build.gradle.kts:697-703` |
| Android defines `REACT_NATIVE_DEBUGGER_ENABLED_DEVONLY` for its merged library too, which makes the default TurboModules serve `ReactDevToolsRuntimeSettingsModule` | doc: `RN/ReactAndroid/src/main/jni/CMakeLists.txt:377-381`, `RN/ReactCommon/react/nativemodule/defaults/DefaultTurboModules.cpp:21-23,87-91` |
| Hermes compiles its CDP agent only with `HERMES_ENABLE_DEBUGGER`, which also turns on memory instrumentation and makes every call check for a pending debugger request | doc: `~/.cache/react-native-tizen/src/hermes-hermes-v260318099.0.4/CMakeLists.txt:233-237,714-716`, `.../API/hermes/CMakeLists.txt:8-50`, `.../lib/VM/Interpreter.cpp:1236-1244` |
| With DevTools on, Android asks Metro for `excludeSource=true&sourcePaths=url-server`: source maps that name files on the server instead of carrying their text | doc: `RA/devsupport/DevServerHelper.kt:298`, `METRO/lib/parseBundleOptionsFromBundleRequestUrl.js:131-143` |
| The Components tab reaches React through `__FUSEBOX_REACT_DEVTOOLS_DISPATCHER__` and a runtime binding the frontend adds over CDP | doc: `RN/Libraries/Core/setUpReactDevTools.js:31-140` |

Host: `devsupport/Inspector.cpp`, `devsupport/InspectorTransport.cpp`, the
target handed to each `ReactInstance` in `ReactHostInstance.cpp`, and
`unregisterFromInspector()` in `ReactHost::stopReactNative`. Its device id
hashes the app, the TV's model and its firmware build (inf: two identical TVs
on the same firmware running the same app replace each other in the proxy).

Verified in the simulator and on a Samsung GQ75LS03DAUXZG (The Frame 2024,
armv7, Tizen 9.0), 2026-11-03: the app is listed in `/json/list` with
`nativePageReloads`; `j` printed "Launching DevTools..." and opened React
Native DevTools on it; a breakpoint in the example's uptime interval paused
the runtime and DevTools put "Paused in debugger" on the TV; evaluation on the
paused frame, `Runtime.consoleAPICalled` and the React DevTools backend's
`operations` (the Components tab's tree) answered through the proxy; `r` while
paused reloaded, and the debugger attached to the new instance. The proxy only
takes a debugger socket whose `Origin` is the server or localhost, which
DevTools' own page has (doc: `InspectorProxy.js:369-387`).

## 10. babel-preset-expo's deep import warning

A development bundle warns once per `import` of `react-native/<path>` from a
file outside `node_modules` (doc:
`EXPO/babel-preset-expo/build/configs/expo.js:19-21,194-207`,
`EXPO/babel-preset-expo/build/plugins/warn-on-deep-rn-imports.js`). Installed
from npm, `react-native-overrides/` sits in `node_modules` and never warns;
the example loads it from the repository, so its Babel config passes
`disableDeepImportWarnings`.
