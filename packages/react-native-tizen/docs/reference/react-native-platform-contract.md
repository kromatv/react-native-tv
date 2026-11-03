# React Native platform contract

What a React Native 0.88 platform host must implement to run bridgeless with
Fabric and Hermes, read from the two reference hosts (Android's `ReactHostImpl`,
iOS's `RCTHost`/`RCTInstance`) and from the out-of-tree platforms that ship
today (react-native-windows, react-native-macos). It ends with what this means
for `runtime/host/src`.

## How to read this page

| Tag | Meaning |
|-----|---------|
| **doc** | Read in the cited source line or page. |
| **inf** | Inferred; the reason is given next to it. |

Nothing here is verified on hardware yet. Host line numbers were read on
2026-11-02 while the host was being edited; re-check them before acting. Paths
are abbreviated:

| Prefix | Path |
|--------|------|
| `RN/` | `node_modules/.bun/react-native-tvos@0.88.0-0+79564db6e6a74bb7/node_modules/react-native-tvos/` (line numbers are pinned to this build) |
| `RC/` | `RN/ReactCommon/react/` |
| `RA/` | `RN/ReactAndroid/src/main/java/com/facebook/react/` |
| `RI/` | `RN/ReactCommon/react/runtime/platform/ios/ReactCommon/` |
| `host/` | `runtime/host/src/` in this package |
| `RNW@` | `https://github.com/microsoft/react-native-windows/blob/bdce17da6a/` (main, RN 0.89 nightly, read 2026-11-02) |
| `CG/` | `node_modules/.bun/@react-native+codegen@0.88.0+2b91fc17bf64bdfd/node_modules/@react-native/codegen/lib/` |
| `SVG/` | `node_modules/.bun/react-native-svg@15.16.0+79564db6e6a74bb7/node_modules/react-native-svg/` |
| `EAL/` | `node_modules/.bun/expo-modules-autolinking@58.0.13+e26be0fe15a52155/node_modules/expo-modules-autolinking/src/` |
| `EXPO/` | `node_modules/.bun/expo@58.0.16+695ac0bd7eaf52d6/node_modules/expo/` |

## 1. Bridgeless startup

### What every host does, in order

| Step | Contract | Source | Host provides |
|------|----------|--------|---------------|
| Feature flags | Overridden once per process, before any flag is read. iOS picks `ReactNativeFeatureFlagsOverridesOSSStable` (or Canary/Experimental) from the release level; Android overrides with `ReactNativeFeatureFlagsOverrides_RNOSS_Stable_Android`. OSS Stable only turns on `useNativeViewConfigsInBridgelessMode`. | doc: `RN/Libraries/AppDelegate/RCTReactNativeFactory.mm:330-347`, `RA/defaults/DefaultNewArchitectureEntryPoint.kt:92-102`, `RC/featureflags/ReactNativeFeatureFlagsOverridesOSSStable.h:16-22` | A provider subclass if it needs any flag the stable set leaves off. Tizen's (`host/HostFeatureFlags.cpp`) is OSS Stable plus `cxxNativeAnimatedEnabled`, for the native animated driver (section 5). |
| JS thread | A `MessageQueueThread` that runs tasks in order. Hermes is created for it. | doc: `RI/RCTInstance.mm:324-329`, `RN/ReactAndroid/src/main/jni/react/runtime/jni/JReactInstance.cpp:61-66` | The thread. |
| Timers | A `PlatformTimerRegistry` feeds `TimerManager`, whose executor is the **buffered** runtime executor. `TimerManager::attachGlobals` installs `setTimeout`, `setInterval`, `requestAnimationFrame` (a 0 ms timer, not a frame callback) and their clears. In bridgeless JS, `queueMicrotask`, `setImmediate` and `requestIdleCallback` come from C++ modules, not from a `Timing` module. | doc: `RI/RCTInstance.mm:311-316,332-333`, `JReactInstance.cpp:44-46,71`, `RC/runtime/TimerManager.cpp:197-363` (rAF at 356-363), `RN/Libraries/Core/setUpTimers.js` (the `RN$Bridgeless === true` branch) | A timer registry. No `Timing` TurboModule is needed. |
| Timer firing | The registry gets `createTimer(id, 0)` for both `setTimeout(0)` and `requestAnimationFrame`, so it cannot tell them apart. Both reference hosts fire every timer on the next display frame, a 0 ms one included: iOS's bridgeless registry calls `createTimerForNextFrame`, which `RCTTiming` fires from its CADisplayLink (intervals under 18 ms run every frame); Android's calls `JavaTimerManager.createTimer`, whose timers fire from a Choreographer frame callback. That is what paces `requestAnimationFrame` to the display. | doc: `RC/runtime/TimerManager.cpp:76-103,356-363`, `RN/ReactCommon/react/runtime/platform/ios/ReactCommon/ObjCTimerRegistry.mm:48-61`, `RN/React/CoreModules/RCTTiming.mm:242-278,379-390`, `RN/ReactAndroid/src/main/jni/react/runtime/jni/JavaTimerRegistry.cpp:18-24`, `RA/modules/core/JavaTimerManager.kt:182-189,285-319` | A frame clock. Tizen's (`host/timers/ThreadTimerRegistry.cpp`) waits for deadlines on a thread and fires expired timers from an Ecore animator (`host/timers/FrameClock.cpp`); a timer no frame has fired 100 ms after it expired fires from the thread, since a window the compositor sends no frames to would otherwise hold it. |
| `ReactInstance` | Owns the runtime, the `RuntimeScheduler`, the `JsErrorHandler` and two executors. A `HostTarget` (inspector) is optional; without it, no CDP debugging. | doc: `RC/runtime/ReactInstance.cpp:67-190` | An `onJsError` callback, optionally a `HostTarget`. |
| Executors | `getBufferedRuntimeExecutor()` holds work until the main bundle has run; `getUnbufferedRuntimeExecutor()` does not. Rule from the source: buffered for anything that calls into JS modules, unbuffered only for installing bindings. iOS hands the surface presenter the buffered one as `runtimeExecutor` and the unbuffered one as `bridgelessBindingsExecutor`; Android registers Fabric with the buffered one and TurboModules with the unbuffered one. | doc: `ReactInstance.cpp:214-234`, `RI/RCTInstance.mm:421-424`, `RA/runtime/ReactInstance.kt:183-186,261-262` | Use them as the reference hosts do. |
| Call invoker | `createJSCallInvoker()` returns a buffered invoker when `enableBufferedCallInvoker` is on (default on). TurboModules get this invoker. | doc: `ReactInstance.cpp:243-254`, `RC/featureflags/ReactNativeFeatureFlagsDefaults.h:88-90` | Nothing. |
| Runtime setup | `initializeRuntime` defines `RN$Bridgeless`, the error globals (`RN$handleException`, `RN$isRuntimeReady`...), `RN$registerCallableModule`, the timers, then runs the host's install function. iOS installs: TurboModule binding, native logger, native component registry binding, and the legacy `UIManager` constants binding when `useNativeViewConfigsInBridgelessMode` is on. | doc: `ReactInstance.cpp:462-670`, `RI/RCTInstance.mm:444-475` | `TurboModuleBinding::install`, `bindNativeLogger`, `bindHasComponentProvider`; `RN$LegacyInterop_UIManager_*` (`RC/runtime/nativeviewconfig/LegacyUIManagerConstantsProviderBinding.cpp`) if the flag stays on. |
| Bundle | `loadScript` evaluates on the JS thread, marks the runtime ready, **then flushes the buffered executor**. It is asynchronous; there is no completion callback, ordering comes from the buffer. | doc: `ReactInstance.cpp:274-339` | Bytes and a source URL, from a file or a dev server. |
| Calls into JS | `callFunctionOnModule` goes through the buffered executor and needs the module registered with `RN$registerCallableModule`. `RCTDeviceEventEmitter`, `RCTNativeAppEventEmitter` and `HMRClient` are registered by `setUpBatchedBridge.js`. | doc: `ReactInstance.cpp:345-395`, `RN/Libraries/Core/setUpBatchedBridge.js:20-40` | Device events through `RCTDeviceEventEmitter.emit`. |
| Surface start | `UIManager::startSurface` runs `RN$AppRegistry.runApplication` through the runtime executor it was given, and throws `Global was not installed` if the bundle has not run. | doc: `RC/renderer/uimanager/UIManager.cpp:235-262`, `RC/renderer/uimanager/AppRegistryBinding.cpp:27-31` | A buffered executor, so a surface can start before the bundle arrives. |

Verified on a 2024 Samsung The Frame (LS03D), Tizen 9.0, armv7, 2026-11-03,
with KROMA's frame HUD, which counts `requestAnimationFrame` callbacks: before
timers waited for a frame it read 5000 to 9000 FPS on every screen; after, 56
to 60, the animator ticking at 60 Hz (`rn-tizen perf`, `hz`).

### Errors

| Path | Contract | Source |
|------|----------|--------|
| Before the runtime is ready, and for any exception caught by the runtime executor | `JsErrorHandler::handleError` calls the host's `onJsError`. iOS shows a RedBox in development and calls `RCTFatal` otherwise. | doc: `ReactInstance.cpp:99-107`, `RI/RCTInstance.mm:634` |
| After the runtime is ready | With `useAlwaysAvailableJSErrorHandling` off (the default, stable does not change it), `RN$handleException` returns `false` and JS reports through the `ExceptionsManager` TurboModule: `reportException(data)` with `data.isFatal`. | doc: `ReactInstance.cpp:567-574`, `RN/Libraries/Core/ExceptionsManager.js:122-132,151-157`, `ReactNativeFeatureFlagsDefaults.h:336-338` |

So a host must treat a fatal `ExceptionsManager.reportException` exactly like a
fatal `onJsError` (inf: the two paths are split only by `isRuntimeReady`).

### Teardown and reload

| Contract | Source |
|----------|--------|
| iOS invalidates every TurboModule and destroys `ReactInstance` **on the JS thread**, then terminates that thread. | doc: `RI/RCTInstance.mm:222-250` |
| `~ReactInstance` quits the `TimerManager` and clears Hermes's event loop control before the scheduler dies. | doc: `ReactInstance.cpp:191-200` |
| Reload is teardown plus a new instance; RNW exposes it as `ReactNativeHost.ReloadInstance`. | doc: https://microsoft.github.io/react-native-windows/docs/native-api/ReactNativeHost |

### Dev server, HMR, LogBox

| Piece | Contract | Source |
|-------|----------|--------|
| Packager status | `GET http://host:port/status` answers `packager-status:running`. | doc: `RA/devsupport/PackagerStatusCheck.kt:81`, `RN/React/Base/RCTBundleURLProvider.mm:117` |
| Bundle URL | `/<entry>.bundle?platform=<os>&dev=true&...`; Metro builds for the `platform` query. | doc: `RNW@vnext/Shared/DevServerHelper.h:112-124` |
| Packager socket | `ws://host:port/message`, JSON messages with `version` 2 and a `method`; hosts handle `reload` and `devMenu`. | doc: `RA/packagerconnection/JSPackagerClient.kt:34,56-61`, `RA/devsupport/DevServerHelper.kt:149-155` |
| HMR | After the bundle loads, native calls `HMRClient.setup(platform, bundleEntry, host, port, isEnabled, scheme)`. Android passes `"android"`, iOS `RCTPlatformName`, RNW its platform name. | doc: `RA/devsupport/DevSupportManagerBase.kt:712`, `RN/React/CoreModules/RCTDevSettings.mm:472-484`, `RNW@vnext/Microsoft.ReactNative/ReactHost/ReactInstanceWin.cpp:624-636`, `RN/Libraries/Utilities/HMRClient.js:152-170` |
| DevSettings | `TurboModuleRegistry.getEnforcing('DevSettings')`. A dev bundle imports it at startup: `setUpDeveloperTools.js` requires `HMRClient`, which requires `DevSettings`. Missing module, dev bundle dies at load. | doc: `RN/src/private/specs_DEPRECATED/modules/NativeDevSettings.js:33`, `RN/Libraries/Core/setUpDeveloperTools.js:21`, `HMRClient.js:17` |
| LogBox | `AppRegistry` registers a `LogBox` surface; JS asks native to show it through the optional `LogBox` module (`show`, `hide`). | doc: `RN/Libraries/ReactNative/AppRegistry.js:15`, `RN/Libraries/LogBox/LogBox.js:98`. inf: native starts a second surface on app key `LogBox`, as the module exists only to do that. |
| Loading banner | Optional `DevLoadingView` (`showMessage`, `hide`). | doc: `RN/src/private/specs_DEPRECATED/modules/NativeDevLoadingView.js:16-22` |
| Inspector | `ws://host:port/inspector/device?...` needs a `HostTarget` handed to `ReactInstance`. | doc: `RA/devsupport/DevServerHelper.kt:123`, `ReactInstance.cpp:112-166` |

RNW's bridgeless host does exactly this list: packager connection, inspector,
then `HMRClient.setup`, and registers `DevSettings` and `LogBox` in C++ (doc:
`RNW@vnext/Microsoft.ReactNative/ReactHost/ReactInstanceWin.cpp:234-340,441-636`).

## 2. Fabric mounting contract

### Scheduler delegate

| Callback | Contract | Source |
|----------|----------|--------|
| `schedulerDidFinishTransaction` | "No-op on all platforms except Android". iOS ignores it. | doc: `RC/renderer/scheduler/Scheduler.cpp:296-299`, `RN/React/Fabric/RCTSurfacePresenter.mm:335-338` |
| `schedulerShouldRenderTransactions` | The mount point. Called at the end of the RuntimeScheduler's loop, or synchronously on the committing thread when `mountSynchronously`. iOS mounts inline when already on the main queue, else hops to it. | doc: `Scheduler.cpp:301-312`, `RCTSurfacePresenter.mm:340-343`, `RN/React/Fabric/Mounting/RCTMountingManager.mm:187-202` |
| `schedulerShouldMergeReactRevision` | Only fires with `enableFabricCommitBranching` (default off). | doc: `RC/renderer/scheduler/SchedulerDelegate.h:43-47`, `ReactNativeFeatureFlagsDefaults.h:116-118` |
| `schedulerDidDispatchCommand` | View commands (`scrollTo`...), run on the main thread against the mounted view. | doc: `RCTMountingManager.mm:204-217` |
| `schedulerShouldSynchronouslyUpdateViewOnUIThread` | Apply raw props to a mounted view now, without a commit; the C++ Animated driver uses it. | doc: `RCTMountingManager.mm:285-330`, `RC/renderer/animated/NativeAnimatedNodesManagerProvider.cpp:72-74` |
| `schedulerDidSetIsJSResponder`, accessibility events, view snapshots | Touch, accessibility and view transitions; iOS leaves the snapshot ones empty. | doc: `RN/React/Fabric/RCTScheduler.mm` (`SchedulerDelegateProxy`) |
| `reportMount(surfaceId)` | After each mounted transaction, iOS calls `scheduler.reportMount` asynchronously; Android does it from `FabricUIManager`. It runs the `UIManagerMountHook`s that IntersectionObserver, MutationObserver and event timing depend on. | doc: `RCTSurfacePresenter.mm:478-485`, `RA/fabric/FabricUIManager.java:1538`, `RC/renderer/uimanager/UIManager.cpp:694-717` |

### Root view and surfaces

| Contract | Source |
|----------|--------|
| The platform creates the root component view (tag = surface id) and attaches it to its own window; Fabric never sends a Create for it. | doc: `RCTMountingManager.mm:164-185` |
| A surface is a `SurfaceHandler` with layout constraints and a `LayoutContext` (`pointScaleFactor`, `fontSizeMultiplier`, `viewportOffset`). `pointScaleFactor` must match `PixelRatio`, which JS reads from `DeviceInfo`. | doc: `RC/renderer/scheduler/SurfaceHandler.h`; inf: rounding in layout uses the factor, JS uses `scale`. |

### Mutations

Order inside one transaction (doc: `RC/renderer/mounting/Differentiator.cpp:1328-1356`):
destructive downward, Update, Remove (reversed), Delete, Create, downward, Insert.
Every removed descendant gets its own Remove before its Delete.

| Type | iOS does | Source |
|------|----------|--------|
| Create | Dequeue a view for the component handle; nothing else. | doc: `RCTMountingManager.mm:53-61` |
| Insert | props, event emitter, state, layout metrics, `finalizeUpdates`, then mount at `index`. | doc: `RCTMountingManager.mm:74-92` |
| Remove | Unmount from parent at `index`. | doc: `RCTMountingManager.mm:94-100` |
| Delete | Recycle the view. | doc: `RCTMountingManager.mm:62-72` |
| Update | Apply only what differs (props, emitter, state, layout), then `finalizeUpdates(mask)`. | doc: `RCTMountingManager.mm:102-134` |

A transaction is pulled with `MountingCoordinator::pullTransaction`, inside the
telemetry controller's will/did-mount hooks, and never re-entered (a nested
request sets a follow-up flag). doc: `RCTMountingManager.mm:234-272`.

### View flattening

| Rule | Source |
|------|--------|
| A `View` forms a native view only if it paints or stacks: background, border, opacity, transform, `collapsable={false}`, `nativeID`, `testID`, events, `display: none`, overflow clipping, accessibility. Others are flattened: their offset folds into the children and they never reach the host. | doc: `RC/renderer/components/view/ViewShadowNode.cpp:44-77` |
| Android TV also keeps `focusable`, `hasTVPreferredFocus` and friends as views; the `cxx` platform has no such props. | doc: `RC/renderer/components/view/platform/android/react/renderer/components/view/HostPlatformViewProps.h:44-61`, `.../platform/cxx/.../HostPlatformViewProps.h:15` |
| `Text` forms a view only on Android; elsewhere only `Paragraph` is mounted and its text arrives as state. | doc: `RC/renderer/components/text/TextShadowNode.h:28-35` |

### Layout metrics

doc: `RC/renderer/core/LayoutMetrics.h:23-53`; iOS applies them in
`RN/React/Fabric/Mounting/UIView+ComponentViewProtocol.mm:84-119`.

| Field | Meaning | Host must |
|-------|---------|-----------|
| `frame` | Origin relative to the parent view that is mounted, size in points. | Position and size. iOS uses center and bounds because `frame` with a transform is undefined. |
| `contentInsets` | Border plus padding. `getContentFrame()` is the box text draws in. | Lay content (text, image) inside it. |
| `borderWidth` | Per edge. | Draw borders. |
| `displayType` | `None` for `display: none`. | Hide the view (iOS sets `hidden`). |
| `layoutDirection` | LTR or RTL. | Mirror if supported. |
| `overflowInset` | How far children overflow. | Hit testing and culling only. |
| `pointScaleFactor` | Points to pixels. | Round to pixels with it. |

`onLayout` is not the host's job any more: `LayoutEventEmitter` is a commit
hook in the core, throttled per view. doc: `Scheduler.cpp:155-157`,
`RC/renderer/uimanager/LayoutEventEmitter.h:17-30`,
`RC/renderer/components/view/BaseViewEventEmitter.cpp:55-80`.

### State

| Component | State the host reads | State the host writes | Source |
|-----------|----------------------|-----------------------|--------|
| Paragraph | Attributed string and paragraph attributes. | None. | doc: `RC/renderer/components/text/ParagraphShadowNode.h` |
| ScrollView | `contentOffset`, content bounding rect. | The live `contentOffset` after every scroll, through `state->updateState`. The core uses it for `getContentOriginOffset`, so `measure` inside a ScrollView is only right if the host writes it. | doc: `RC/renderer/components/scrollview/ScrollViewState.h:26-56`, `ScrollViewShadowNode.cpp:61-79`, `RN/React/Fabric/Mounting/ComponentViews/ScrollView/RCTScrollViewComponentView.mm:684-705` |

### Commands

| Component | Commands (JS `supportedCommands`) | Source |
|-----------|-----------------------------------|--------|
| View | `focus`, `blur`, `hotspotUpdate`, `setPressed`, `setDestinations`, `requestTVFocus` | doc: `RN/Libraries/Components/View/ViewNativeComponent.js:37-44` |
| ScrollView | `flashScrollIndicators`, `scrollTo(x, y, animated)`, `scrollToEnd(animated)` (and `zoomToRect`, iOS only) | doc: `RN/Libraries/Components/ScrollView/ScrollViewCommands.js:45-52`, `RCTScrollViewComponentView.mm:957-959` |
| AndroidTextInput | `focus`, `blur`, `setTextAndSelection(mostRecentEventCount, text, start, end)`; see `text-input.md` | doc: `RN/Libraries/Components/TextInput/AndroidTextInputNativeComponent.js:644-646` |

A command can arrive before the transaction that created its view has mounted
(inf: the command is dispatched on the JS thread during the same loop whose
rendering update mounts later), so a host queues commands behind pending mounts
or drops them only for tags it will never see.

### Events

| Event | Emitted by | Payload | Source |
|-------|-----------|---------|--------|
| `onLayout` | Core commit hook | `{layout: {x, y, width, height}}` | doc: see Layout metrics |
| `onFocus`, `onBlur`, `onPressIn`, `onPressOut` | Host, through `BaseViewEventEmitter` | empty | doc: `RC/renderer/components/view/BaseViewEventEmitter.h:34-39` |
| `onScroll`, `onScrollBeginDrag`, `onScrollEndDrag`, `onMomentumScrollBegin/End` | Host, through `ScrollViewEventEmitter`, throttled by `scrollEventThrottle` | `contentOffset`, `contentInset`, `contentSize`, `layoutMeasurement`, `zoomScale`, `timestamp` (ms), `responderIgnoreScroll` | doc: `RC/renderer/components/scrollview/ScrollEvent.cpp:12-47`, `RCTScrollViewComponentView.mm:784-793` |
| `onTextLayout` | Core, during layout, if the `TextLayoutManager` supports `measureLines`; otherwise a warning. | lines | doc: `RC/renderer/components/text/ParagraphShadowNode.cpp:351-360` |

Events are queued on the `EventDispatcher` and flushed when the `EventBeat`
fires; a beat runs as RuntimeScheduler work, or synchronously on the calling
thread for `experimental_flushSync`. doc: `RC/renderer/core/EventBeat.cpp:44-79`,
`RC/renderer/core/EventQueue.cpp:85-91`.

### measure and friends

`measure`, `measureInWindow`, `measureLayout`, `getBoundingClientRect` are
answered in C++ from the shadow tree; the host contributes only the
ScrollView offset (above) and `viewportOffset` for `measureInWindow`. doc:
`RC/renderer/uimanager/UIManagerBinding.cpp:519-716`,
`RC/renderer/dom/DOM.cpp:536-550`.

## 3. TurboModules

`getEnforcing` throws at the import of the spec when the module is missing; `get`
returns null. doc: `RN/Libraries/TurboModule/TurboModuleRegistry.js:39-47`.
Specs live in `RN/src/private/specs_DEPRECATED/modules/`.

| Module (JS name) | Lookup | Events (device events unless noted) | Needed by | Tizen |
|------------------|--------|-------------------------------------|-----------|-------|
| `PlatformConstants` | get | none | `Platform` (every app) | yes |
| `DeviceInfo` | getEnforcing | `didUpdateDimensions` with `{window, screen}`, read by `Dimensions` | every app | yes, no events |
| `SourceCode` | getEnforcing | none; `scriptURL` resolves dev assets | every app | yes |
| `ExceptionsManager` | getEnforcing | none | every app | yes; a fatal report takes the host's fatal path |
| `AppState` | getEnforcing | `appStateDidChange` `{app_state}`, `memoryWarning`, `appStateFocusChange` (bool) | every app | yes |
| `DeviceEventManager` | get | none; `invokeDefaultBackPressHandler` | `BackHandler` | yes |
| `Appearance` | get | `appearanceChanged` | `useColorScheme` | yes |
| `Networking` | getEnforcing | see below | `fetch`, `XMLHttpRequest` | yes |
| `ImageLoader` | getEnforcing | none | `<Image>` | stub |
| `WebSocketModule` | getEnforcing | `websocketOpen`, `websocketMessage`, `websocketClosed`, `websocketFailed` | `WebSocket`, the HMR client | yes, see WebSocket below |
| `BlobModule` | get | none, but it changes `fetch` (below) | `Blob`, `URL.createObjectURL` | yes, see Blob below |
| `FileReaderModule` | getEnforcing | none | `FileReader`, `response.blob()` | yes |
| `LinkingManager` | get | `url` `{url}` | `Linking` on any `Platform.OS` that is not `android` | yes: `getInitialURL` answers the launching app control's URI; `openURL` rejects, `canOpenURL` answers false |
| `I18nManager` | get | none | RTL; JS falls back to LTR constants | no |
| `StatusBarManager` | getEnforcing, at module scope | none | importing `StatusBar` (expo-status-bar does) | yes, height 0, setters do nothing |
| `DevSettings` | getEnforcing | `didPressMenuItem` | dev bundles | no |
| `TVNavigationEventEmitter` | get | `onHWKeyEvent` | `TVEventHandler`, `TVEventControl` | events only, as device events |
| `Timing` | get | none | legacy only; bridgeless timers are C++ | not needed |

Sources for the rows: `RN/Libraries/AppState/AppState.js:87-147`,
`RN/Libraries/Utilities/Dimensions.js:125-132`,
`RN/Libraries/WebSocket/WebSocket.js:148-279`,
`RN/Libraries/Linking/Linking.js:26,51-93`,
`RN/src/private/specs_DEPRECATED/modules/NativeStatusBarManagerAndroid.js:29`.
C++ defaults already answer `NativeReactNativeFeatureFlags`, `NativeMicrotasks`,
`NativeIdleCallbacks`, `NativeDOM`, `NativePerformance` and, behind flags, the
observers and `NativeAnimatedModule`. doc: `RC/nativemodule/defaults/DefaultTurboModules.cpp:30-90`.

React Native modules KROMA still looks up and the host does not serve (its
simulator log, 2026-11-03), none of them called on `tizen`:

| Module | Why nothing breaks | Source |
|--------|--------------------|--------|
| `SoundManager` | `playTouchSound` runs only when `Platform.OS === 'android'` | doc: `RN/Libraries/Pressability/Pressability.js:482,823`, `RN/Libraries/Components/Touchable/Touchable.js:813` |
| `FrameRateLogger` | every call is `NativeFrameRateLogger?.` | doc: `RN/Libraries/Interaction/FrameRateLogger.js:42-60` |
| `KeyboardObserver` | handed to `NativeEventEmitter` on iOS only | doc: `RN/Libraries/Components/Keyboard/Keyboard.js:76` |
| `AccessibilityInfo`, `IntentAndroid` | used on `android` only; `tizen` takes the iOS branch, served by `AccessibilityManager` and `LinkingManager` | doc: `RN/Libraries/Components/AccessibilityInfo/AccessibilityInfo.js:50`, `RN/Libraries/Linking/Linking.js:26,51` |
| `TVNavigationEventEmitter` | required on iOS only; elsewhere TV events are device events | doc: `RN/Libraries/Components/TV/TVEventHandler.js:50` |

Every event above reaches JS through `RCTDeviceEventEmitter` because
`NativeEventEmitter` only routes through the module on iOS. doc:
`RN/Libraries/EventEmitter/NativeEventEmitter.js:73-88,116-117`.

### Networking, checked against the current host

Tizen resolves `RCTNetworking.android.js`: it calls
`sendRequest(method, url, requestId, headers as [name, value][], data, responseType, useIncrementalUpdates, timeout, withCredentials)`
and listens on device events. doc: `RN/Libraries/Network/RCTNetworking.android.js:40-44,82-92`.
Each event's single argument is an array (doc:
`RN/Libraries/Network/RCTNetworkingEventDefinitions.flow.js`,
`XMLHttpRequest.js:574-600`).

| Event | Payload | Host (`host/modules/NetworkingModule.cpp`) | Verdict |
|-------|---------|-------------------------------------------|---------|
| `didReceiveNetworkResponse` | `[id, status, headers, url]` | `:43` | matches |
| `didReceiveNetworkData` | `[id, body]` (text, or base64 for `arraybuffer`) | `:52` | matches |
| `didReceiveNetworkIncrementalData` | `[id, chunk, progress, total]` | `:48-50`, only for `text` with incremental updates, as one chunk at the end | shape matches; never streams (`host/network/HttpEngine.cpp:159-161`) |
| `didReceiveNetworkDataProgress` | `[id, loaded, total]` | never sent | gap |
| `didSendNetworkData` | `[id, progress, total]` | never sent | gap |
| `didCompleteNetworkResponse` | `[id, error, timedOut]` | `:57` | matches |

React Native's `fetch` is `whatwg-fetch` 3.6.20 over XHR. It sets
`responseType = 'blob'` when `new Blob()` works, else `'arraybuffer'`, which XHR
sends to native as `base64`. `new Blob()` throws without `BlobModule`; with it,
every `fetch` goes as `blob`, and `Networking` answers with a blob descriptor
`{blobId, offset, size, type}` (`host/modules/NetworkingModule.cpp`, as iOS's
`RCTBlobManager` handles `blob` responses). doc:
`node_modules/.bun/whatwg-fetch@3.6.20/node_modules/whatwg-fetch/fetch.js:12-23,593-600`,
`RN/Libraries/Blob/BlobManager.js:57,66`, `RN/Libraries/Blob/Blob.js:60-63`,
`RN/Libraries/Network/XMLHttpRequest.js:228-231,604-610`.

### WebSocket, checked against the host

`host/modules/websocket/WebSocketModule.cpp` over `host/network/WebSocketConnection.cpp`.

| Fact | Source |
|------|--------|
| JS calls `connect(url, protocols, {headers, unstable_devToolsRequestId}, socketID)`, `send(text, id)`, `sendBinary(base64, id)`, `ping(id)`, `close(code, reason, id)` | doc: `RN/src/private/specs_DEPRECATED/modules/NativeWebSocketModule.js:16-33` |
| Each event is a device event with one object: `websocketOpen {id, protocol}`, `websocketMessage {id, type, data}` (`text`, `binary` in base64, or `blob` with a `BlobData`), `websocketClosed {id, code, reason}`, `websocketFailed {id, message}`; JS turns a failure into `error` then `close` with 1006 | doc: `RN/Libraries/WebSocket/WebSocket.js:61-72,240-293` |
| Android adds `Origin: http(s)://host[:port]` when JS sets none, drops a protocol that is empty or holds a comma, joins the rest with `,`, and gives the connection 10 s | doc: `RA/modules/websocket/WebSocketModule.kt:113,138-165,468-492` |
| `send`, `sendBinary` and `ping` on an unknown socket answer `websocketFailed` with `client is null`, then `websocketClosed` with code 0; `close` on one does nothing | doc: `WebSocketModule.kt:276-306` |
| `ping` sends a Ping frame on iOS, an empty binary message on Android; Tizen sends a Ping frame | doc: `RN/React/CoreModules/RCTWebSocketModule.mm:191-193`, `WebSocketModule.kt:408` |
| Once `BlobModule.addWebSocketHandler(id)` ran, binary messages arrive as `{type: 'blob', data: {blobId, offset: 0, size}}` | doc: `RA/modules/blob/BlobModule.kt:48-62` |
| The Tizen 9 rootstrap's libcurl is 8.5.0, older than 8.11.0, the release that took curl's WebSocket support out of experimental, so the host does not count on `curl_ws_send` (I). Tizen 10's rootstrap carries 8.11.0. So the host speaks RFC 6455 itself over a connection libcurl opens with `CURLOPT_CONNECT_ONLY` (DNS, TCP, TLS, proxy) | `rs:usr/include/curl/curlver.h:35`, the same for the Tizen 10 rootstrap, and curl's 8.11.0 changelog (https://curl.se/ch/8.11.0.html), 2026-11-03 |
| Over TLS, libcurl offers HTTP/2 through ALPN unless `CURLOPT_HTTP_VERSION` is 1.1; a server that picks h2 then has `curl_easy_recv` decode HTTP/2 frames. The RFC 6455 upgrade is HTTP/1.1 only | verified in simulator: a SIGSEGV in `nghttp2_session_mem_recv2` under `curl_easy_recv` on `wss://` before the option was set, 2026-11-03; RFC 6455 section 4.1 |
| A client sends a random 16-byte key, checks status 101, `Upgrade`, `Connection` and `Sec-WebSocket-Accept`, masks every frame, fails on a masked server frame, answers a Ping with a Pong and a Close with a Close; control frames carry at most 125 bytes and are never fragmented; an empty Close reads as 1005 | doc: RFC 6455 sections 4.1, 4.2.2, 5.1, 5.3, 5.5, 5.5.1, 7.1.5 |

Verified in simulator (`example/src/screens/modules`, 2026-11-03): the open
event with the protocol the server picked and the headers and `Origin` it
received, text in UTF-8, a 70000-byte binary message, ping, a server close with
4001, a client close with 1000, binary messages as blobs, sending a Blob, `wss://`
to a public echo, and a refused connection failing with 1006. KROMA's events
socket connects: its cast receiver leaves the server's roster within 2 s of the
app stopping, and a cast command from the server reaches the TV.

### Blob and FileReader, checked against the host

| Fact | Source |
|------|--------|
| iOS answers `BLOB_URI_SCHEME: 'blob'`, `BLOB_URI_HOST: null`; Android answers only with a content provider declared. `URL.createObjectURL` throws without a scheme. Tizen answers as iOS does | doc: `RN/Libraries/Blob/RCTBlobManager.mm:69-75`, `RA/modules/blob/BlobModule.kt:133-141`, `RN/Libraries/Blob/URL.js:17-30,70-75` |
| iOS's networking answers `blob:` URLs from the blob store; Tizen's `Networking` does too | doc: `RCTBlobManager.mm:220-223` |
| `createFromParts` gets `{type: 'string' \| 'blob', data}` parts; strings are stored as UTF-8 | doc: `BlobModule.kt:298-320`, `RN/Libraries/Blob/BlobManager.js:62-100` |
| JS gives each Blob the host object `global.__blobCollectorProvider(blobId)` returns; its destruction frees the bytes. Android installs it from `BlobModule`'s JSI bindings | doc: `BlobManager.js:42-49`, `RN/ReactAndroid/src/main/jni/react/reactnativeblob/BlobCollector.cpp:56-80` |
| `readAsDataURL` answers `data:<type, or application/octet-stream>;base64,...`; `readAsText(blob, encoding)` decodes with that charset, `UTF-8` by default. Tizen decodes UTF-8, ISO-8859-1 and US-ASCII | doc: `RA/modules/blob/FileReaderModule.kt:49,80-86`, `RN/Libraries/Blob/FileReader.js:185` |
| `expo/fetch`'s `response.blob()` stores the bytes with `ExpoFetchModule.unstable_createBlobData`, then wraps the id with `BlobManager.createFromOptions`; Android stores them in its `BlobModule`. Tizen's `ExpoFetchModule` uses the same store | doc: `EXPO/src/winter/fetch/createBlob.ts:16-26`, `EXPO/android/src/main/java/expo/modules/fetch/ExpoFetchModule.kt:54-58` |
| Expo replaces the global `URL`, and its `createObjectURL` reads the constants through `globalThis.__turboModuleProxy('BlobModule')` when `RN$Bridgeless` is true. Bridgeless React Native never defines `__turboModuleProxy`, so the global `URL.createObjectURL` throws `undefined is not a function`. React Native's own `URL` (`react-native/Libraries/Blob/URL`) works | doc: `EXPO/src/winter/runtime.native.ts:16`, `EXPO/src/winter/url.ts:34-56`, `RC/nativemodule/core/ReactCommon/TurboModuleBinding.cpp:117-150`; verified in simulator, 2026-11-03. inf: the same on bridgeless Android and iOS, whose hosts install the binding the same way |

Verified in simulator (2026-11-03): `new Blob` from strings read back as text,
`readAsDataURL`, `slice`, React Native's `URL.createObjectURL` fetched by XHR,
an XHR `blob` response, an XHR upload of a Blob, React Native's own `fetch`
reading UTF-8 text and a blob, and `expo/fetch`'s `blob()`.

### What each common path needs

| App path | Modules |
|----------|---------|
| Start at all (release) | PlatformConstants, DeviceInfo, SourceCode, ExceptionsManager, AppState, ImageLoader once `<Image>` loads, Networking once `fetch` is touched |
| Start a dev bundle | the above plus DevSettings; WebSocketModule for HMR; LogBox and DevLoadingView to see errors and progress |
| `fetch` / XHR | Networking; Blob plus FileReader only if `BlobModule` exists |
| Deep links | LinkingManager (`getInitialURL` at startup, `url` events) |
| React Navigation, expo-router | LinkingManager (inf: both read `Linking.getInitialURL` on start) |
| `useWindowDimensions` | DeviceInfo, plus `didUpdateDimensions` if the size can change |
| Expo template | StatusBarManager (expo-status-bar imports `StatusBar`) |

## 4. react-native-tvos specifics

### `onHWKeyEvent`

| | Android TV | tvOS | Source |
|-|-----------|------|--------|
| Route | `emitDeviceEvent("onHWKeyEvent", map)` from the root view's key handler | notification forwarded by `TVNavigationEventEmitter` | doc: `RA/ReactAndroidHWInputDeviceHelper.kt:25-50`, `RN/React/CoreModules/RCTTVNavigationEventEmitter.mm` |
| `eventType` | `select` (DPAD_CENTER, ENTER, SPACE), `up/down/left/right`, `play`, `pause`, `playPause`, `rewind`, `fastForward`, `stop`, `next`, `previous`, `info`, `menu`, `channelUp`, `channelDown` | the same names plus `longSelect`, `longPlayPause`, `long{Up,Down,Left,Right}`, `pageUp/Down`, `swipe*`, `pan`, `focus`, `blur` | doc: `ReactAndroidHWInputDeviceHelper.kt:59-77`, `RN/React/Base/RCTTVRemoteHandlerConstants.mm:8-32`, `RN/React/Base/RCTTVNavigationEventNotification.mm:12,17` |
| `eventKeyAction` | `0` on key down and `1` on key up: two events per press | gesture state: began `0`, ended `1`, else absent; a tap arrives once with `1` | doc: `ReactAndroidHWInputDeviceHelper.kt:27-32`, `RCTTVNavigationEventNotificationConstants.mm:16-18`, `RCTTVNavigationEventNotification.mm:67-79` |
| `tag` | the last focused native view | the focused view, when any | doc: `ReactAndroidHWInputDeviceHelper.kt:43-48` |
| Back | not an `onHWKeyEvent`; goes to `BackHandler` (`hardwareBackPress`) | menu key through `BackHandler` | doc: `RN/README.md:241` |
| Back with a `Modal` shown | the Modal's dialog takes Back and Escape at `ACTION_UP` and dispatches `onRequestClose` (`RequestCloseEvent`); the key is consumed, so `BackHandler` never sees it | | doc: `RA/views/modal/ReactModalHostView.kt:291-321`, `RA/views/modal/ReactModalHostManager.kt:93-96` |

JS listens through `TVEventHandler.addListener('onHWKeyEvent')`, on device
events off iOS. doc: `RN/Libraries/Components/TV/TVEventHandler.js:48-70`.

### Focus

| Feature | Native contract | Source |
|---------|-----------------|--------|
| `focusable`, `hasTVPreferredFocus`, `nextFocus{Up,Down,Left,Right,Forward}`, `autoFocus`, `trapFocus*` | View props parsed on Android only; they drive Android's and UIKit's focus engines. | doc: `HostPlatformViewProps.h:44-61` (android), `RN/README.md:250-263` |
| `TVFocusGuideView` | A `View` with `autoFocus`/`trapFocus*`, plus the View command `setDestinations` on TV. | doc: `RN/Libraries/Components/TV/TVFocusGuideView.js:93-160` |
| VirtualizedList | On TV, wraps its content in a `TVFocusGuideView` with `trapFocus*`. | doc: `RN/README.md:347` |
| Select on a focused Pressable | Android TV dispatches `topPressIn`/`topPressOut` on DPAD_CENTER/ENTER down/up to the focused view; Pressability drops a TV click with no `eventType`. | doc: `RA/views/view/ReactViewGroup.kt:1397-1437`, `RN/Libraries/Pressability/Pressability.js:604-610` |
| `TVEventControl` | `enable/disableTVMenuKey`, `enable/disableTVPanGesture`, `enable/disableGestureHandlersCancelTouches`; Apple TV only, each a no-op when `TVNavigationEventEmitter` is missing. | doc: `RN/Libraries/Components/TV/TVEventControl.js`, `RN/README.md:243-247` |

A JS-driven focus engine (KROMA's `@kromatv/spatial-nav` is a tree walker, not
a geometry search) needs only three things from the host: key events, a way to
scroll (`scrollTo`), and an app that does not also rely on the native focus
engine. It ignores `focusable`, `nextFocus*` and `TVFocusGuideView`, so their
absence on Tizen costs it nothing (inf: from [`packages/spatial-nav/README.md`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/spatial-nav/README.md)
and [`packages/ui/src/lib/seam-keys.ts:36-40`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/lib/seam-keys.ts#L36-L40), which treats a key as released
only on Android). The wrappers stay harmless: a `TVFocusGuideView` is a plain
View and its `setDestinations` command is dropped.

## 5. Animated native driver

| Piece | Contract | Source |
|-------|----------|--------|
| Module | The C++ `AnimatedModule` answers `NativeAnimatedModule` from the defaults only when `cxxNativeAnimatedEnabled` is on and, off Android, `useSharedAnimatedBackend` too. Both default off. | doc: `DefaultTurboModules.cpp:74-85`, `ReactNativeFeatureFlagsDefaults.h:40-42,352-354` |
| JS fallback | With no `NativeAnimatedModule`/`NativeAnimatedTurboModule`, `useNativeDriver: true` warns once ("the native animated module is missing") and runs in JS, and every Animated component's mount-time `API.flushQueue()` throws "Native animated module is not available" unless `cxxNativeAnimatedEnabled` is on. | doc: `RN/src/private/animated/NativeAnimatedHelper.js:43-44,222-262,427-452`, `RN/src/private/animated/createAnimatedPropsHook.js:75-82` |
| JS with `cxxNativeAnimatedEnabled` | Brackets each batch with `startOperationBatch`/`finishOperationBatch` and skips the mount-time flush; never uses `NativeAnimatedTurboModule`. | doc: `NativeAnimatedHelper.js:56-72`, `RN/Libraries/Animated/shouldUseTurboAnimatedModule.js:14-20` |
| Frame loop | `NativeAnimatedNodesManager` takes start and stop callbacks; the platform runs `onRender` every frame between them. Start comes from the JS thread (`isAsync`) or the render thread; stop from `onRender` itself, or from the manager's destructor. iOS builds the provider itself with a `CADisplayLink`, and holds the provider strongly while a frame runs. | doc: `RC/renderer/animated/NativeAnimatedNodesManager.h:60-70`, `NativeAnimatedNodesManager.cpp:583-640,1175-1242`, `RN/ReactApple/RCTAnimatedModuleProvider/RCTAnimatedModuleProvider.mm:76-150` |
| Legacy backend | Props reach views two ways: `synchronouslyUpdateViewOnUIThread` (delegate callback, section 2) for each frame, and `updateShadowTree` commits through the JS thread for layout props and final values; a mounting override delegate set on every surface keeps a later commit from undoing animated props. iOS applies a direct update by cloning the mounted view's props through its component descriptor. | doc: `NativeAnimatedNodesManagerProvider.cpp:55-131`, `RC/renderer/animated/internal/AnimatedMountingOverrideDelegate.cpp:34-104`, `RN/React/Fabric/Mounting/RCTMountingManager.mm:285-330` |
| Shared backend | With `useSharedAnimatedBackend`, the `AnimationBackend` on the UIManager owns commits. | doc: `NativeAnimatedNodesManagerProvider.cpp:133-140` |
| Ecore animators | An animator runs once per frame: the clock starts at 1/60 s and Elementary sets it to `1 / fps` (60 by default). On Wayland the tick is the compositor's frame callback. `ecore_animator_del` only marks the animator and returns its data, so a callback may delete its own animator; if it then also returns `ECORE_CALLBACK_CANCEL`, the deletion is counted twice. | src: `efl:src/lib/ecore/ecore_anim.c#L80`, `#L494-L503`, `#L847-L870`, `efl:src/lib/elementary/elm_config.c#L1603`; `efl-rendering.md` section 1 |

How Tizen wires it: `host/HostFeatureFlags.cpp` turns on
`cxxNativeAnimatedEnabled`; `host/animated/NativeAnimatedModule.cpp` serves
`NativeAnimatedModule` with a provider whose start and stop callbacks drive
`host/animated/AnimationFrameLoop.cpp`, an Ecore animator calling `onRender` on
the main loop that holds the provider while a frame runs, as iOS does;
`SurfacePresenter::schedulerShouldSynchronouslyUpdateViewOnUIThread` applies
the props through `host/animated/AnimatedViewProps.cpp`, the iOS way, with the
component handle `MountingManager` records for each view. Layout animations
still need an `animationDelegate` given to the `Scheduler`.

Verified in simulator (`example/src/screens/modules`, 2026-11-03): with
JavaScript blocked for 3 s by a busy loop, a `useNativeDriver: true`
`translateX` and `opacity` animation keeps moving while its
`useNativeDriver: false` twin stops; the host logs `Native animated: 362 frames
in 6030 ms on the main loop` for the 6 s animation, and the view ends where the
final commit puts it. KROMA no longer logs the missing-module warning.

## 6. How out-of-tree platforms resolve JavaScript

| Mechanism | react-native-windows | react-native-macos | react-native-tizen |
|-----------|----------------------|--------------------|--------------------|
| Kind | Separate package with overrides of RN's `Libraries/` | Full fork with inline diffs | Metro resolver over the app's own `react-native` |
| Declared in | `react-native.config.js` `platforms.windows.npmPackageName: 'react-native-windows'` (`RNW@vnext/react-native.config.js:34-41`) | same, `platforms.macos` and `visionos` (`react-native-macos .../react-native.config.js:178-216`) | `withTizen` in the app's `metro.config.js` (`src/metro/with-tizen.ts:80-107`) |
| Redirect | CLI's `reactNativePlatformResolver` rewrites `react-native` and `react-native/x` to the package for that platform; `resolver.platforms` gets the platform; `<pkg>/setup-env` runs before main (`facebook/react-native packages/community-cli-plugin/src/utils/metroPlatformResolver.js:27-48`, `loadMetroConfig.js:38-73`) | same | `.tizen.*` wins, else resolve as `android` and swap files present in `react-native-overrides/` (`with-tizen.ts:86-97`) |
| `Platform.OS` | `'windows'`, in `Platform.windows.js` derived from `Platform.android.js`; `isTV` false; `select` checks `windows`, `native`, `default` (`RNW@vnext/src-win/Libraries/Utilities/Platform.windows.js:13-74`, `overrides.json:671-676`) | `'macos'`, in-tree `Platform.macos.js` | `'tizen'`; `isTV` true; `select` checks `tizen`, `android`, `native`, `default` (`react-native-overrides/Libraries/Utilities/select-for-tizen.ts`), and so does Expo's build-time inlining once `withTizen` wraps the Babel transformer (`src/metro/tizen-platform-select.ts`) |
| Platform files | 124 overrides of four kinds: platform, derived, patch, copy, managed by `react-native-platform-override` (`RNW@vnext/overrides.json`, `RNW@packages/react-native-platform-override/README.md:40-91`) | `*.macos.js` beside upstream files | four derived overrides (`Platform`, `Alert`, the ScrollView and Image view configs) and a native module wherever the iOS branch works with one |

The CLI docs define `npmPackageName` as the package that "should provide a
complete react-native implementation for that platform" (doc:
https://github.com/react-native-community/cli/blob/f4c4ef37cc/docs/platforms.md#L47-L51).
reactnative.dev's Out-of-Tree Platforms page still describes the old rnpm/Haste
registration and says the process "is still not very well documented" (doc:
https://reactnative.dev/docs/out-of-tree-platforms). RN 0.89 nightlies no longer
ship a `react-native.config.js`, so RNW re-exports the community `bundle` and
`start` commands itself (doc: `RNW@vnext/react-native.config.js:10-16`).

### The cost of "Android files, Tizen OS"

Files resolve as Android, but runtime checks compare `Platform.OS` with
`'android'` or `'ios'`, and `'tizen'` fails both. In RN's JS, 30 files test
`'android'` and 36 test `'ios'` (doc: a grep of `RN/Libraries` and
`RN/src/private`, tests excluded). RNW avoids this by owning a copy of every file
whose branch matters.

In a production bundle, `babel-preset-expo` also replaces every static
`Platform.select({...})` with the entry for the bundle's platform, then
`native`, then `default`, before any `Platform` module runs (doc:
`node_modules/.bun/babel-preset-expo@58.0.18+94fda58f5b22cbd6/node_modules/babel-preset-expo/build/configs/expo.js:38-47`,
`.../build/plugins/minify-platform-select-plugin.js:53-66`). Expo's transform
worker skips Metro's own inline pass (doc:
`node_modules/.bun/@expo+metro-config@58.0.15+4264611429502de5/node_modules/@expo/metro-config/build/transform-worker/metro-transform-worker.js:304`).
For Tizen that picked `default` over `android` until `withTizen` renamed the
`android` entry (verified in simulator: `Platform.select({ios, android, default})`
answered `default`, then `android`).

What each file does on `tizen` and what this package does about it (a read of
every file in `RN/Libraries` and `RN/src/private` that compares `Platform.OS`
with `'android'` or `'ios'`; Tizen loads `ProgressBarAndroid.android.js`, the
only one with an Android twin):

| File | Branch on `tizen` | Effect before | Now |
|------|-------------------|---------------|-----|
| `Linking/Linking.js:26,51-93` | the iOS one | `getInitialURL` throws from `nullthrows` | `LinkingManager` answers; verified in simulator: `getInitialURL` resolves `null`, an `open-url` arrives as `url` |
| `Alert/Alert.js:100-110` | neither | `Alert.alert` does nothing | `Alert.tizen.ts` takes the Android path; `DialogManagerAndroid` draws a native dialog the remote drives; verified in simulator (each button, Back on a cancelable alert) |
| `Components/StatusBar/StatusBar.js:15-16` | neither, but imports both specs | the import throws without `StatusBarManager` | `StatusBarManager` answers; verified in simulator |
| `Vibration/Vibration.js:14` | `isTV` guards | the import throws without `Vibration` | `Vibration` answers, doing nothing; verified in simulator |
| `Components/AccessibilityInfo/AccessibilityInfo.js:50-501` | the iOS one | every query rejects | `AccessibilityManager` answers every setting off; verified in simulator (`isReduceMotionEnabled` resolves false) |
| `Components/ScrollView/ScrollViewNativeComponent.js:21` | the iOS one | the iOS view config drops `horizontal`, `scrollsChildToFocus`, `isInvertedVirtualizedList`, `removeClippedSubviews`... | `ScrollViewNativeComponent.tizen.ts`, Android's config |
| `Image/ImageViewNativeComponent.js:60` | the iOS one | `Image.android.js` runs with the iOS config: `src`, `headers`, `shouldNotifyLoadEvents` dropped, a `defaultSource` URI resolved to null | `ImageViewNativeComponent.tizen.ts`, Android's config |
| `Share/Share.js:98-120` | neither | rejects "Unsupported platform" | unchanged: safer than Android's path |
| `Components/TextInput/TextInput.js:75,625,953`, `TextInputState.js:105,136` | neither | renders nothing, no focus | `src/metro/tizen-android-branch.ts` reads `Platform.OS` as `android` in both files; `AndroidTextInput` is a host component (`text-input.md`); verified in simulator |
| `Utilities/DevSettings.js:14` | any | a `__DEV__` bundle throws without the module | open: the dev loop gap below |
| `WebSocket/WebSocket.js:30` | any | the first `WebSocket` throws without the module | `WebSocketModule` answers; verified in simulator |
| `Lists/FlatList.js:166`, `Components/ScrollView/ScrollView.js:1873,1883` | the iOS one | `removeClippedSubviews` off by default; `snapToAlignment` children flattened | open, low |
| `EventEmitter/NativeEventEmitter.js:73`, `Network/RCTNetworking.android.js:43` | non-iOS | device events, as on Android | correct |
| RN's `Platform.select({android, ios})` without `default` (13 files, e.g. `Components/Button.js:317-348`, LogBox fonts) | `undefined` | unstyled values | the Android value, at build and run time |

## 7. Third-party native libraries and codegen

What `rn-tizen` relies on to build a library's native code
([`../native-modules.md`](../native-modules.md)).

| Fact | Source |
|------|--------|
| A library declares its specs in package.json: `codegenConfig` with `name` (the C++ namespace and include directory), `type` (`components`, `modules`, `all`) and `jsSrcsDir`, plus optional per-platform keys | doc: `SVG/package.json` (`codegenConfig`) |
| `combineSchemasInFileList(files, platform, exclude, libraryName)` keeps files named `Native*` or `*NativeComponent`; given a platform, a file with a platform suffix (`.android.ts`) counts only for that platform | doc: `CG/cli/combine/combine-js-to-schema.js:42-62`, `CG/cli/combine/combine-utils.js:4-22` |
| Android generates a library with `componentsAndroid` (descriptors, events, props, states, shadow nodes, Java) and `modulesAndroid` plus `modulesCxx`; `modulesCxx` is one header, `<name>JSI.h`, with the `Native<Spec>CxxSpec` bases a C++ TurboModule derives from | doc: `CG/generators/RNCodegen.js:52-64`, `CG/generators/modules/GenerateModuleH.js:471-475` |
| A component declared `interfaceOnly` gets props and events but no shadow node, state or descriptor: the library writes those in its own C++ | doc: `CG/generators/components/GenerateShadowNodeH.js:72`, `GenerateComponentDescriptorH.js:58`, `GenerateStateH.js:49` |
| `excludedPlatforms` filters only the Objective-C++ and Java generators; the C++ generators write every component | doc: `CG/generators/components/GenerateComponentHObjCpp.js:289`, `GeneratePropsJavaDelegate.js:233` |
| Android links a library's descriptors through `react-native.config.js` (`dependency.platforms.android.componentDescriptors`, `cmakeListsPath`); react-native-svg's C++ is `common/cpp`, compiled with the codegen output | doc: `SVG/react-native.config.js`, `SVG/android/src/main/jni/CMakeLists.txt:10-29` |
| react-native-svg renders `RNSVGSvgViewAndroid` only when `Platform.OS === 'android'`: on `tizen` it renders `RNSVGSvgView`, so both need a native view | doc: `SVG/src/elements/Svg.tsx:190` |
| react-native-svg sends a brush as `{type, payload}` (0 a processed color, 1 a `url(#id)` reference, 2 `currentColor`), `null` for `none`, and black when a renderable sets no fill; `propList` names the presentation props a node sets, which its children inherit | doc: `SVG/src/lib/extract/extractBrush.ts`, `SVG/src/lib/extract/extractFill.ts`, `SVG/src/lib/extract/extractProps.ts:89-95`, `SVG/apple/RNSVGRenderable.mm:727-750` |
| Expo's autolinking walks `dependencies` and non-optional `peerDependencies` at every depth (up to 9), and `devDependencies` of the app only | doc: `EAL/dependencies/resolution.ts:17-72` |

## Implications for react-native-tizen

### Gaps, in priority order

| P | Gap | Why it matters | Contract to meet |
|---|-----|----------------|------------------|
| 1 | View commands and a scrolling ScrollView | KROMA's rails and lists scroll with `scrollTo` ([`packages/ui/src/components/organisms/virtual/virtual-rail/virtual-rail.tsx:145,173`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/components/organisms/virtual/virtual-rail/virtual-rail.tsx#L145), [`packages/ui/src/components/organisms/command/use-cursor-scroll.ts:22`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/components/organisms/command/use-cursor-scroll.ts#L22)). Fixed: commands reach their view after the commit that mounted it, and `host/views/ScrollView.cpp` applies the offset, writes `ScrollViewState` and emits `onScroll` and `onMomentumScrollEnd`; verified in simulator (`efl-rendering.md`, ScrollView). | Section 2 Commands, State, Events: apply the offset, write `ScrollViewState`, emit `onScroll` with the `ScrollEvent` payload. |
| 2 | Native animated driver | KROMA asks for `useNativeDriver: true` ([`packages/ui/src/components/organisms/toast/toast-card.tsx:30`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/components/organisms/toast/toast-card.tsx#L30), [`packages/ui/src/lib/splash-motion.ts:41`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/lib/splash-motion.ts#L41), [`packages/ui/src/lib/native-film.tsx:78`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/lib/native-film.tsx#L78)) and got JS animation plus a warning. Fixed: React Native's C++ driver runs on an Ecore animator and moves mounted views without JavaScript (section 5); verified in simulator. | Section 5. |
| 2 | Dev loop | The host reads `bundleUrl`, but the CLI never writes it (`src/package/runtime-config.ts:15-33`) and bundles are always `--dev false` (`src/bundle/bundle-app.ts:44-45`). A dev bundle also needs DevSettings, `HMRClient.setup('tizen', ...)`, the `/message` socket and LogBox. Fix bug 1 first. | Section 1 Dev server. |
| 3 | `reportMount` | No IntersectionObserver, MutationObserver or event timing. | Section 2 Scheduler delegate. |
| 3 | WebSocketModule; then Blob and FileReader | HMR and app sockets; Blob changes `fetch` (section 3). Fixed: the host serves all three, so the `setUpXHR` override that hid `WebSocket` is gone; verified in simulator (section 3, WebSocket and Blob). | Section 3. |
| 3 | `onTextLayout` | `TextLayoutManager` has no `measureLines`. | Section 2 Events. |
| 3 | Legacy UIManager constants binding | The OSS Stable flags turn on `useNativeViewConfigsInBridgelessMode`, but the binding iOS and Android install with it is missing, so `UIManager.getViewManagerConfig` raises a soft error (`RN/Libraries/ReactNative/BridgelessUIManager.js:168-173`). Install it or turn the flag off. | Section 1 Runtime setup. |

### Bugs and contract violations in the current host

| # | Where | What | Evidence | Severity |
|---|-------|------|----------|----------|
| 1 | `host/fabric/SurfacePresenter.cpp:21-24` | Fabric's `runtimeExecutor` is `runtimeScheduler->scheduleWork`, which is unbuffered. With a `bundleUrl`, the bundle is downloaded on the JS thread (`host/ReactHostInstance.cpp:136-152`) while `runApplication` is already queued, so `AppRegistryBinding` throws `Global was not installed` and the app never starts. Latent: nothing writes `bundleUrl` yet. Pass `instance_->getBufferedRuntimeExecutor()`. Fixed: `ReactHost` hands Fabric `getBufferedRuntimeExecutor()`. Verified in simulator: the app starts from a `bundleUrl` served over HTTP. | doc: `RI/RCTInstance.mm:421-424`, `RA/runtime/ReactInstance.kt:261-262`, `AppRegistryBinding.cpp:27-31` | fixed |
| 2 | `host/modules/ExceptionsManagerModule.cpp:39-63` | A fatal `reportException`/`reportFatalException` only logs: no error screen in development, no exit in release, unlike `onJsError` (`host/ReactHostInstance.cpp:69-74,154-166`). A render error after startup leaves a frozen screen. Fixed: a fatal report calls the host's `reportFatalError`. Verified in simulator: a render error shows the red screen in development and leaves the app in release. | doc: section 1 Errors | fixed |
| 3 | `host/ReactHost.cpp:72-80` | `instance_.reset()` destroys Hermes on the main thread while the JS thread can still be running a task; the thread is stopped only afterwards. iOS destroys the instance on the JS thread. Fixed: the surface stops on the main thread while the JS thread waits between tasks (`TaskQueueThread::runWhileParked`), then TurboModules and the instance die on the JS thread, as on iOS. | doc: `RI/RCTInstance.mm:230-250` | fixed |
| 4 | `host/ReactHostInstance.cpp:84-88`, `host/ReactHostWindow.cpp:36-39` | The `emitDeviceEvent` captured by every module dereferences `instance_`. `modules_` (and with it the network thread, `host/modules/NetworkingModule.h:38`) outlives `instance_.reset()`, so a response completing during shutdown dereferences null. Fixed: every device event goes through an `InstanceGate` the host closes before teardown. Verified in simulator: six restarts with a request in flight, no crash. | inf: member order in `host/ReactHost.h:85-87` and the destructor body | fixed |
| 5 | `host/ReactHost.cpp:56`, `host/ReactHostInstance.cpp:69-74,154-158` | `onJsError` runs on the JS thread and `reportFatalError` copies `lifetime_` there, while the destructor resets the same `shared_ptr` on the main thread: a read and a write of one `shared_ptr` object at once is a data race. Fixed: callbacks capture a `weak_ptr` copied on the main thread; only the main thread touches `lifetime_`. | inf: `std::shared_ptr` is not safe for concurrent access to one instance | fixed |
| 6 | `host/text/EvasTextMeasurer.cpp:49-53` | Layout on the JS thread blocks on the main loop for every measurement `UnwrappedSizes` cannot answer, though no longer behind a whole mount: the main loop runs it between two mutations (`runOnMainThreadSoon`). Evas cannot measure off the main loop (efl-rendering.md section 7). Shutdown is safe now: `closeMainThreadSync` releases waiters before the join (`host/threading/MainThread.cpp:131-138`, `host/ReactHost.cpp:59`). A synchronous event beat still deadlocks: `executeNowOnTheSameThread` on the main thread (`EventBeat.cpp:76`) waits for the JS thread, which waits for the main loop. Latent while the host dispatches no events. | inf: two threads each waiting on the other | low (latent) |
| 7 | `host/fabric/SurfacePresenter.cpp:77-85` | Mounts from `schedulerDidFinishTransaction` as well as `schedulerShouldRenderTransactions`, so commits mount before the scheduler's rendering update and every commit costs two main-thread hops. `runOnMainThread` always queues (`host/threading/MainThread.cpp:36-38`), so a `mountSynchronously` commit never mounts inline. | doc: `Scheduler.cpp:296-312`, `RCTSurfacePresenter.mm:335-343` | low |
| 8 | `host/fabric/SurfacePresenter.cpp:92-111` | `schedulerDidDispatchCommand` and `schedulerShouldSynchronouslyUpdateViewOnUIThread` are empty: every command and every native-driven prop update is dropped silently. Fixed for commands: they run on the main thread once the commit their view came with is mounted (gap P1). `schedulerShouldSynchronouslyUpdateViewOnUIThread` applies the native animated driver's props to the mounted view (section 5). | doc: section 2 | fixed |
| 9 | `host/mounting/MountingManager.cpp:36-42` | No `reportMount` after a transaction, and no telemetry hooks. | doc: `RCTSurfacePresenter.mm:478-485` | low |
| 10 | `host/views/ComponentView.cpp:103-110` | `displayType`, `borderWidth`, `layoutDirection` and transforms are ignored, and no view ever uses its event emitter (no focus, blur, press or scroll events). `display: none` stays invisible only because Yoga zeroes the subtree (`RN/ReactCommon/yoga/yoga/algorithm/CalculateLayout.cpp:666-674`). Rendering detail belongs in `efl-rendering.md`. | doc: section 2 Layout metrics | medium |
| 11 | `host/ReactHostWindow.cpp:54-56` | `onHWKeyEvent` carries `eventKeyAction: 0` on key down only and no `tag`; `host/input/RemoteInput.cpp:13` listens to `ECORE_EVENT_KEY_DOWN` alone. Neither Android's two-event shape nor tvOS's single `1`. Fixed in favour of tvOS's shape (`docs/decisions.md`): one event with `1` at release, `long<Type>` `0` then `1`; see `tizen-input.md`, section 12. `tag` is still missing. | doc: section 4 | fixed |
| 12 | `host/ReactHost.cpp:107-109` | A URL from `app_control` is emitted as a `url` event; before JS subscribes (the buffer flushes right after the bundle, before `runApplication`) it is lost, and there is no `getInitialURL` to recover it. Fixed: the shell passes the first app control's URI to `rn_tizen_host_set_initial_url`, which `LinkingManager.getInitialURL` answers; later ones arrive as `url`. | doc: `ReactInstance.cpp:331-334`, `Linking.js:90-93` | fixed |
| 13 | `host/modules/NetworkingModule.cpp` | A `formData` or `uri` body is sent as no body instead of failing the request (a `blob` body is sent since `BlobModule` exists). A POST with no body goes out through `CURLOPT_CUSTOMREQUEST` without `POSTFIELDS` (`host/network/HttpEngine.cpp:127-133`), so curl sends no `Content-Length`. | inf: libcurl semantics, not tested | low |
| 14 | `host/network/HttpEngine.cpp:159-161` | The body is delivered once, after the transfer ends: incremental XHR (`onprogress`, streaming responses) sees nothing until completion. | doc: `XMLHttpRequest.js:570-571` | low |
| 15 | `host/HostFeatureFlags.cpp:13-25` with `host/fabric/SurfacePresenter.cpp:37` | OSS Stable turns on native view configs, but the binding they need is never installed (gap P3), and the `Scheduler` gets no animation delegate, so `LayoutAnimation` is a no-op. | doc: `RI/RCTInstance.mm:457-459`, `RA/runtime/ReactInstance.kt:219-224` | low |
| 16 | `host/fabric/CoreComponents.cpp:48-58`, `host/views/ComponentView.cpp:94` | Every mounted view's props are `static_cast` to `ViewProps`. Safe for the registered components and the unimplemented fallback, all `ViewProps` subclasses; a future component whose props are not would be undefined behaviour. | doc: the registry at `CoreComponents.cpp:21-37` | low |

What already matches the contract: timers on the buffered executor
(`host/ReactHostInstance.cpp:77`), modules on `createJSCallInvoker`
(`host/ReactHostInstance.cpp:109`), the unbuffered `bridgelessBindingsExecutor`
(`host/ReactHostInstance.cpp:120`), events through `callFunctionOnModule`
(`host/ReactHostWindow.cpp:36-39`), main-thread waiters released before the JS
thread is joined (`host/threading/MainThread.cpp:131-138`, `host/ReactHost.cpp:59`), Create/Insert/Remove/Delete/Update applied in
the order received, `onLayout` left to the core, `Back` routed to `BackHandler`
or, with a `Modal` mounted, to the last one's `onRequestClose`
(`host/views/ModalHostView.cpp`; verified in simulator),
and the Networking event shapes above.
