# Expo modules contract

What Expo SDK 58 needs from a platform before its JavaScript runs, and what
react-native-tizen gives it today. Read it before touching `runtime/host/expo/`.

Versions read: expo-modules-core 58.0.23, expo 58.0.16, expo-asset 58.0.17,
expo-constants 58.0.12, expo-font 58.0.9, expo-file-system 58.0.10, expo-device
58.0.5, expo-keep-awake 58.0.3, expo-blur 58.0.5, expo-image 58.0.16, expo-video
58.0.12, expo-modules-autolinking 58.0.13, @expo/config-plugins 58.0.9,
react-native-tvos 0.88.0-0. Docs read on 2026-11-02.

## How facts are tagged

| Tag | Meaning |
|-----|---------|
| **doc** | Documented on docs.expo.dev; the page is linked. |
| **src** | Read in the installed source at the cited file and line. Expo does not document it, so it can change in a patch release. |
| **inf** | Inferred; the reasoning follows the claim. Verify before relying on it. |

Paths: `emc/` is expo-modules-core, `expo/` is the expo package, `rn/` is
react-native-tvos, all as installed in `node_modules`. Other packages are named in
full. `runtime/` and `src/` are this package. `tv-native/` is KROMA's Apple TV and
Android TV client, [`clients/tv-native`](https://github.com/kromatv/kroma/tree/1d270635adcc9f3a81283e04f06336845d8c5985/clients/tv-native) at commit `1d27063`, the first app this
runtime targets.

## 1. `globalThis.expo`

expo-modules-core's JavaScript reads `globalThis.expo` at import time. iOS
installs it in `AppContext.prepareRuntime` (`emc/ios/Core/AppContext.swift:478-515`)
in this order: the core object, `__expo_app_identifier__`, the CoreModule members,
`EventEmitter`, `SharedObject` (with a releaser that deletes from the registry),
`SharedRef`, `NativeModule`, then `modules`. **src**

| Member | Read by | If missing | Tizen today |
|--------|---------|------------|-------------|
| `EventEmitter` class | `emc/src/EventEmitter.ts:21` at import; base of the next three (`emc/common/cpp/NativeModule.cpp:10-13`, `SharedObject.cpp:25-26`) | Exported as `undefined`; the first `new` or `extends` on it throws | Installed (`runtime/host/expo/ExpoRuntime.cpp`, `installExpoGlobals`) |
| `NativeModule` class | `emc/src/NativeModule.ts:11` | Same | Installed |
| `SharedObject` class | `emc/src/SharedObject.ts:11`; `instanceof` in `NativeViewManagerAdapter.native.tsx:85` | Same | Installed; its releaser deletes from `SharedObjectRegistry` |
| `SharedRef` class | `emc/src/SharedRef.ts:13` | Same | Installed |
| `modules` | First lookup of `requireOptionalNativeModule` (`emc/src/requireNativeModule.ts:39`) | Falls back to `NativeModulesProxy`, then `TurboModuleRegistry.get` wrapped as an Expo module (`:40-41`) | Host object over `ExpoModuleRegistry` (`ExpoModulesHostObject.cpp`) |
| `modules.NativeModulesProxy` | `emc/src/NativeModulesProxy.native.ts:13` | Used only when `NativeModules.NativeUnimoduleProxy` exists (`:24`) | Absent, correct |
| `uuidv4()` | `emc/src/uuid/index.ts:5-15` | `uuid.v4()` throws | Installed (`Uuid.cpp`) |
| `uuidv5(name, namespace)` | `emc/src/uuid/index.ts:17-33` | `uuid.v5()` throws | Installed: SHA-1, RFC 4122 section 4.3 (`Uuid.cpp`, `Digest.cpp`) |
| `getViewConfig(module, view?)` | `NativeViewManagerAdapter.native.tsx:61` | Warns; the view keeps only base View props (section 2) | Answers from the view registry (`views/ExpoViewRegistry.cpp`), null for an unknown view |
| `reloadAppAsync(reason)` | `emc/src/reload.ts:10`, exported by `expo` | `reloadAppAsync()` rejects with a TypeError | Returns a promise rejected with `ERR_UNAVAILABLE`: the host has no reload path yet |
| `expoModulesCoreVersion` | Typed as `{version, major, minor, patch}` (`emc/src/ts-declarations/global.ts:57-64`); no reader in the installed tree (searched) | Nothing today | That object |
| `cacheDir`, `documentsDir` | Typed only (`global.ts:71-78`); no reader found | Nothing today | `file://` URIs with a trailing slash, Android's shape |
| `isRunningOnQuest` | `emc/src/Platform.ts:82`, defaults to `false` | `false` | Missing, harmless |
| `installOnUIRuntime(holder)` | `emc/src/worklets/index.ts:8-16`, for react-native-worklets | Throws when called | Missing |
| `__expo_app_identifier__` | Suffixes native view names (`NativeViewManagerAdapter.native.tsx:53-58`); set only for apps with an identifier (`AppContext.swift:491-493`) | No suffix | Absent, correct |

If `globalThis.expo` itself is missing, `ensureNativeModulesAreInstalled` calls the
TurboModule `ExpoModulesCore.installModules()` (`emc/src/ensureNativeModulesAreInstalled.native.ts:7-21`). **src**
Tizen registers that TurboModule (`runtime/host/expo/ExpoModulesCoreModule.cpp`):
it receives React Native's CallInvoker and the host's `ModuleContext`, and the
host's `installExpoRuntime(runtime)` calls its `installModules()` while the
runtime initializes, so the global exists before the bundle runs.

The values iOS and Android return: `expoModulesCoreVersion` is an object
(`emc/ios/Core/Modules/CoreModule.swift:11-21`, `emc/android/.../defaultmodules/CoreModule.kt:28-42`);
`cacheDir` is a path on iOS and a `file://` URI with a trailing slash on Android
(`CoreModule.swift:23-29`, `CoreModule.kt:44-50`); `reloadAppAsync` is an
AsyncFunction that reloads the same bundle (`CoreModule.swift:75-77`). **src**
"Reloads the app. This method works for both release and debug builds" and it
"does not use a new update even if one is available"
([SDK reference](https://docs.expo.dev/versions/latest/sdk/expo/), Methods). **doc**

### Classes and events in C++

| Mechanism | Contract | Source |
|-----------|----------|--------|
| Class creation | `common::createClass` evaluates, once per runtime, a JS factory whose constructor is `function (...args) { return this.__native_constructor__(...args); }`, and installs the native constructor on the prototype | `emc/common/cpp/JSI/JSIUtils.cpp:13-58` **src** |
| JS subclasses | A JS `class X extends NativeClass` reaches native through `super()`, with `this` being the JS instance. Expo relies on it: `FetchResponse extends NativeResponse`, `File extends FileSystemFile`, `Directory extends FileSystemDirectory` | `expo/src/winter/fetch/FetchResponse.ts:5,160-176`; `expo-file-system/src/File.ts:45`, `Directory.ts:18,31-33` **src** |
| Listener methods | `addListener`, `removeListener`, `removeAllListeners`, `emit`, `listenerCount` on the EventEmitter prototype | `EventEmitter.cpp:321-336` **src** |
| Observing | The first listener for a name calls `__expo_onStartListeningToEvent` then `startObserving(name)` on the emitter; removing the last calls the stop pair | `EventEmitter.cpp:138-171` **src** |
| Emitting | `expo::EventEmitter::emitEvent(runtime, object, name, args)` | `EventEmitter.h:119-124` **src** |
| Threading | `emitEvent` takes the `jsi::Runtime`, so it must run on the JS thread | **inf**: a `jsi::Runtime` is not thread-safe |

### SharedObject and SharedRef lifecycle

| Step | What happens | Source |
|------|--------------|--------|
| Id | The registry's `nextId` starts at 1 and increments under a mutex. 0 means "not paired" | `emc/ios/Core/SharedObjects/SharedObjectRegistry.swift:39-42,72-79` **src** |
| Pairing | Native sets `expo::SharedObject::NativeState(objectId, releaser)` on the JS object and records id to native object. iOS also defines an own `__expo_shared_object_id__` and, for a SharedRef, `nativeRefType` | `SharedObjectRegistry.swift:95-163`; `emc/common/cpp/SharedObject.h:45-62` **src** |
| Id seen by JS | A prototype getter returns the native state's id, or 0 | `SharedObject.cpp:98-107` **src** |
| As a view prop | `processPropValue` replaces a SharedObject with its numeric id. expo-video passes `player={id}` explicitly | `NativeViewManagerAdapter.native.tsx:84-111`; `expo-video/src/VideoView.tsx:90,102-105` **src** |
| `release()` | Calls `releaser(id)`, then clears the native state (ignored on a frozen object) | `SharedObject.cpp:29-50` **src** |
| GC | The NativeState destructor calls `releaser(id)` | `SharedObject.cpp:18-20` **src** |
| Delete | Marks released, `sharedObjectWillRelease`, id reset to 0, entry removed, `sharedObjectDidRelease` | `SharedObjectRegistry.swift:169-183`; Android `SharedObjectRegistry.kt:92` **src** |
| After release | "A function that detaches the JS and native objects to let the native object deallocate before the JS object gets deallocated by the JS garbage collector"; later native calls "will throw an error" | [SDK reference](https://docs.expo.dev/versions/latest/sdk/expo/), `SharedObjectType.release()` **doc** |
| Automatic release | "A shared object is deallocated automatically once neither JavaScript nor native holds a reference" | [Using shared objects](https://docs.expo.dev/modules/shared-objects/) **doc** |
| Hooks | `useReleasingSharedObject*` call `release()` on unmount or a dependency change | `emc/src/hooks/useReleasingSharedObjectWithLifecycle.ts:136,170` **src** |

`readyForJSFinalization` is not a SharedObject API in SDK 58. It is an event of
ExpoFetch's `NativeResponse` (`expo/src/winter/fetch/NativeRequest.ts:21-26`),
emitted last, after the body completed or failed
(`expo/ios/Expo/Fetch/NativeResponse.swift:69,201,258`), and `FetchResponse`
subscribes to it in its constructor to finalize (`FetchResponse.ts:175`). A
search of every installed package finds it nowhere else. **src**

## 2. Native views

| Step | Contract | Source |
|------|----------|--------|
| 1 | `requireNativeView` is `requireNativeViewManager` | `expo/src/Expo.ts:20` **src** |
| 2 | The component name is `ViewManagerAdapter_<module>` or `ViewManagerAdapter_<module>_<view>`, plus `_<appIdentifier>` when set | `NativeViewManagerAdapter.native.tsx:53-58`; iOS `ViewModuleWrapper.swift:134-145` **src** |
| 3 | `NativeComponentRegistry.get(name, provider)`; bridgeless uses the static config from `provider` first | `rn/Libraries/NativeComponent/NativeComponentRegistry.js:51-70` **src** |
| 4 | `provider` calls `expo.getViewConfig(module, view)` and gives every attribute `process: processPropValue`. A null config logs a warning and returns `{ uiViewClassName }` | `NativeViewManagerAdapter.native.tsx:60-77,104-111` **src** |
| 5 | `createViewConfig` merges the platform's base View attributes with the provided ones | `rn/Libraries/NativeComponent/ViewConfig.js:21-42` **src** |
| 6 | With no config, every custom prop (`source`, `player`, `intensity`) is dropped before Fabric | **inf**: React Native's attribute diff only sends keys listed in `validAttributes` |
| 7 | Fabric finds a ComponentDescriptor by that name. On Tizen an unknown name gets `UnimplementedNativeViewComponentDescriptor` and draws empty | `runtime/host/src/fabric/CoreComponents.cpp:48-57`, `CoreComponents.h:9-11` **src** |
| 8 | `ViewPrototypes['<module>_<view>']` on the module is copied onto the wrapper class; its functions find the view through `this.nativeTag` | `NativeViewManagerAdapter.native.tsx:222-261` **src** |

`getViewConfig` returns `{ validAttributes: { <prop>: true }, directEventTypes:
{ top<Name>: { registrationName: 'on<Name>' } } }`, or null for an unknown module
or view. With no view name it looks up the key `DEFAULT_MODULE_VIEW`
(`CoreModule.swift:49-73`, `CoreModule.kt:70-98`, `ModuleDefinition.swift:3`). An
event name that starts with `on` becomes `top...`
(`android/.../events/KModuleEventEmitterWrapper.kt:130-136`). **src**

### The Fabric C++ in `emc/common/cpp/fabric`

| Piece | Role | Source |
|-------|------|--------|
| `ExpoViewComponentDescriptor<>` | One `ConcreteComponentDescriptor` type for every Expo view: the component name and handle come from the descriptor's **flavor** string | `ExpoViewComponentDescriptor.h:14-31` **src** |
| `ExpoViewProps` | `ViewProps` plus `propsMap`, name to `folly::dynamic`, copied from the previous props and overwritten with the updated raw props | `ExpoViewProps.h`, `ExpoViewProps.cpp` **src** |
| `ExpoViewEventEmitter::dispatch(name, payloadFactory)` | Public `dispatchEvent`; React Native turns `onLoad` into `topLoad` | `ExpoViewEventEmitter.cpp`; `rn/ReactCommon/react/renderer/core/EventEmitter.cpp:27-37` **src** |
| `ExpoViewState` | Width, height and style size native may push; `adopt()` writes them into the Yoga style | `ExpoViewState.h`; `ExpoViewComponentDescriptor.h:59-120` **src** |
| `ExpoViewShadowNode` | `ConcreteViewShadowNode`; a measurable leaf only for SwiftUI host views (`sizesToContent`) | `ExpoViewShadowNode.h:25-130` **src** |

Registration: iOS returns one `ComponentDescriptorProvider` per view class, with a
flavor cached for the process "so that the component handle stay the same" after a
reload (`emc/ios/Fabric/ExpoFabricViewObjC.mm:176-196`). Android adds providers to
`CoreComponentsRegistry::sharedProviderRegistry()`, building each descriptor with
`RawPropsParser(useRawPropsJsiValue=true)`
(`emc/android/src/main/cpp/fabric/FabricComponentsRegistry.cpp`,
`ExpoComponentDescriptorFactory.cpp:13-31`). **src**

How Tizen hosts Expo views (`runtime/host/expo/views/`):

1. `runtime/cmake/Expo.cmake` compiles `common/cpp/fabric/*.cpp`.
2. A view is an `ExpoViewType` (module, view, prop setters, event names, factory),
   written with `ViewDefinition<View>` and registered with `RN_TIZEN_EXPO_VIEW`.
3. Registration adds a `ComponentDescriptorProvider` whose flavor is the component
   name and whose constructor builds `expo::ExpoViewComponentDescriptor<>`, as
   Android does, and a `ComponentView` factory under the same name. Both go through
   the host's registries (`registerComponentDescriptor`, `registerComponentView`).
4. `ExpoView` (a `ComponentView`) diffs `ExpoViewProps::propsMap` against what it
   applied and calls one setter per changed prop, then `didUpdateProps()`.
5. `getViewConfig` answers from the same `ExpoViewType`, so the two cannot disagree.
6. `ExpoView::dispatchEvent("onLoad", payload)` goes through the
   `ExpoViewEventEmitter`, which React Native normalizes to `topLoad`.
7. A shared object prop arrives as its id; `SharedObjectRegistry::shared().find<T>(id)`
   resolves it from the main thread. expo-video's `player` is the first user.
8. A view's methods are `ViewDefinition::asyncFunction("enterFullscreen",
   &VideoView::enterFullscreen)`. When JavaScript first reads a module that defines
   views, the host sets `ViewPrototypes` on it, keyed as step 8 of the table above
   (`views/ViewPrototypes.cpp`); each function reads `this.nativeTag`, finds the
   mounted view with `ExpoView::find(tag)` on the main thread and settles its promise
   after the method returns. Verified in simulator with expo-video's
   `enterFullscreen` and `exitFullscreen`.

| View KROMA renders | Component name | Props native receives | Events |
|--------------------|----------------|-----------------------|--------|
| expo-image `Image` (`expo-image/src/ExpoImage.tsx:13`) | `ViewManagerAdapter_ExpoImage` | `source` (array of `{uri, width, height, headers, cacheKey}`), `contentFit`, `transition`, `cachePolicy`, `recyclingKey`, `allowDownscaling`, `placeholder`, `tintColor`, border colors (`ios/ImageModule.swift:37-148`) | `onLoadStart`, `onProgress`, `onError`, `onLoad`, `onDisplay` (`:29-35`). `onLoad` is `{cacheType, source: {url, width, height, mediaType, isAnimated}}` (`src/Image.types.ts:818-836`) |
| expo-video `VideoView` (`expo-video/src/NativeVideoView.ts:4-6`: `VideoView` unless `Platform.OS` is `android`) | `ViewManagerAdapter_ExpoVideo_VideoView` | `player` (shared object id), `nativeControls`, `contentFit`, `contentPosition`, `fullscreenOptions`, `allowsPictureInPicture` (`ios/VideoModule.swift:39-92`) | `onPictureInPictureStart`, `onPictureInPictureStop`, `onFullscreenEnter`, `onFullscreenExit`, `onFirstFrameRender` (`:31-37`) |
| expo-blur `BlurView` | `ViewManagerAdapter_ExpoBlur_ExpoBlurView` and `..._ExpoBlurTargetView` | `blurTargetId`, `intensity`, `tint`, `blurReductionFactor`, `borderRadii`, `blurMethod` (`android/.../BlurModule.kt:12-45`) | none |

expo-blur resolves its Android file on Tizen: the Metro resolver keeps a
`.tizen.*` match and otherwise resolves for `android`
(`src/metro/with-tizen.ts:10,86-98`), so `NativeBlurModule.android.tsx:3,5` asks for
both views. **src**

## 3. Startup of `expo`

| Step | Code | Native touched |
|------|------|----------------|
| Entry | `registerRootComponent` imports `Expo.fx` first (`expo/src/launch/registerRootComponent.tsx:1`) | |
| Winter runtime | `Expo.fx.tsx:2` loads `winter/runtime.native.ts`: React Native's setup, then lazy globals `TextDecoderStream`, `TextEncoderStream`, `URL`, `URLSearchParams`, `DOMException`, `__ExpoImportMetaRegistry`, `structuredClone`, the FormData patch, `Symbol.asyncIterator` (`:6-33`) | none |
| Global fetch | Replaced by `expo/fetch` unless `EXPO_PUBLIC_USE_RN_FETCH` is `1` or `true` (`runtime.native.ts:35-47`) | `ExpoFetchModule` |
| Async require | `Expo.fx.tsx:3`; in development reads `NativeSourceCode.scriptURL` (`expo/src/utils/getBundleUrl.native.ts:7-8`) | `SourceCode` |
| expo-asset | `Expo.fx.tsx:4`; `requireNativeModule('ExpoAsset')` at import (`expo-asset/src/ExpoAsset.ts:3`), optional `ExpoUpdates`, `ExpoGo` in a try (`PlatformUtils.ts:8-16`), imports expo-constants (`:1`) | `ExpoAsset`, `ExpoUpdates`, `ExpoGo`, `ExponentConstants` |
| Dev socket | `Expo.fx.tsx:13-19`, development only | none |
| Error recovery | Only in Expo Go: wraps `ErrorUtils`' global handler (`Expo.fx.tsx:21-25`). Otherwise React Native's own handler and `ExceptionsManager` | `ExpoGo` |
| Dev client | `NativeModules.EXDevLauncher` silences a LogBox warning (`Expo.fx.tsx:30-33`); bundle-splitting progress uses `NativeModules.DevLoadingView` (`devLoadingViewEmitter`, `DevLoadingViewNativeModule.native.ts:3`) | `EXDevLauncher`, `DevLoadingView` |
| Placeholder | Outside production, registers `main` as `AppEntryNotFound` (`Expo.fx.tsx:35-38`) | none |
| Root | Outside production wraps the app in `withDevTools`, which keeps the screen awake through an optional `expo-keep-awake` (`withDevTools.tsx:10-17`); registers `main` (`registerRootComponent.tsx:42-47`) | `ExpoKeepAwake` (development) |

The host must run the surface `main`. Bare apps must make
`getMainComponentName` return `"main"`
([SDK reference](https://docs.expo.dev/versions/latest/sdk/expo/), Common questions). **doc**

### ExpoFetchModule

"On Android and iOS, `expo/fetch` is also installed as the global `fetch`." "To
keep React Native's built-in `fetch` as the global, set `EXPO_PUBLIC_USE_RN_FETCH=1`."
"Named imports from `expo/fetch` continue to work regardless of this flag"
([SDK reference](https://docs.expo.dev/versions/latest/sdk/expo/), `expo/fetch` API). **doc**

| Member | Contract | Source |
|--------|----------|--------|
| `new NativeRequest(response)` | Holds the `FetchResponse` it fills | `fetch.ts:63`; `ExpoFetchModule.swift:98-100` **src** |
| `request.start(url, init, body)` | `body` is `Uint8Array` or null; resolves when the response head arrives, a rejection becomes a `FetchError` | `fetch.ts:86-93`; `NativeRequest.ts:5-12` **src** |
| `init` | `{credentials: 'include' by default, headers: [name, value][], method, redirect: 'follow' by default}` | `NativeRequest.ts:14-19`; `fetch.ts:70-75` **src** |
| `request.cancel()` | Called on abort, after the body stream is errored | `fetch.ts:80-85` **src** |
| `NativeResponse` | Constructed through `super()` by `FetchResponse` | `FetchResponse.ts:5,173-176` **src** |
| Response properties | `bodyUsed`, `_rawHeaders`, `status`, `statusText`, `url`, `redirected` | `NativeRequest.ts:29-34`; `ExpoFetchModule.swift:54-72` **src** |
| Response methods | `startStreaming(): Promise<Uint8Array or null>`, `cancelStreaming(reason)`, `arrayBuffer()`, `text()` | `NativeRequest.ts:35-38` **src** |
| Response events | `didReceiveResponseData(Uint8Array)`, `didComplete()`, `didFailWithError(string)`, then `readyForJSFinalization()` | `NativeRequest.ts:21-26`; `NativeResponse.swift:240-258` **src** |
| `unstable_createBlobData(bytes)` | Returns a React Native BlobModule id, for `response.blob()`. Tizen stores the bytes in the host's `BlobStore`, which `BlobModule` reads (verified in simulator) | `createBlob.ts:18`; `ExpoFetchModule.swift:27-38` **src** |

### ExponentConstants

The module is optional (`expo-constants/src/ExponentConstants.ts:3`). Missing, it
logs "No native ExponentConstants module found" (`Constants.ts:33-37`) and
`Constants.expoConfig` is null. **src**

| Constant | iOS / Android | Consumer |
|----------|---------------|----------|
| `manifest` | The embedded `app.config`: an object on iOS (`ios/EXConstantsObjC/EXConstantsService.m:48`), a JSON string on Android (`ConstantsService.kt:40`) | `Constants.ts:68-80`, then `expoConfig` |
| `executionEnvironment` | `"bare"` on both | `getManifest` warns in `bare` and throws in `standalone` or `storeClient` when there is no manifest (`Constants.ts:222-246`) |
| `sessionId`, `statusBarHeight`, `deviceName`, `systemFonts`, `debugMode`, `isHeadless`, `systemVersion`, `platform.{ios or android}` | Spread into `Constants` (`Constants.ts:84-90`) | apps |

A manifest from `ExpoUpdates` (`manifest` or `manifestString`) or from
`NativeModules.EXDevLauncher.manifestString` wins over this one
(`Constants.ts:39-66`). `app.config` is written at build time by
`expo-constants/scripts/getAppConfig.js`, which calls `getConfig(projectRoot,
{isPublicConfig: true})` and writes the JSON. **src** `ExpoGo`, `ExpoUpdates` and
`EXDevLauncher` are all optional for a release app. **src**

## 4. The modules the TV app uses

A module read with `requireNativeModule` at the top of a file throws "Cannot find
native module" (`emc/src/requireNativeModule.ts:16-23`), so the first import of
its package fails and the app does not start. Tizen registers every module below.

| Package | Native module | Missing on Tizen | TV app usage |
|---------|---------------|------------------|--------------|
| expo-font | `ExpoFontLoader`, required at import (`src/ExpoFontLoader.ts:38`); `ExpoFontUtils` optional | Import throws | `useFonts(KIT_FONTS)` (`tv-native/src/App.tsx:93`) |
| expo-file-system | `FileSystem`, required at import (`src/ExpoFileSystem.ts:48`); `File` and `Directory` extend its classes at import | Import throws | `Paths`, `File`, `Directory` (`tv-native/src/lib/storage.ts:15-37,92`) |
| expo-device | `ExpoDevice`, required at import (`src/ExpoDevice.ts:2`) | Import throws | `Device.deviceName` (`App.tsx:42`) |
| expo-keep-awake | `ExpoKeepAwake`, required at import (`src/ExpoKeepAwake.ts:2`) | Import throws | `useKeepAwake()` (`App.tsx:70`) |
| expo-blur | None; views only. The adapter's `requireNativeModule` is in a try (`NativeViewManagerAdapter.native.tsx:247-261`) | Renders as an empty unimplemented view | `BlurView` as the kit's frost (`App.tsx:62`) |
| expo-image | `ExpoImage`, required at import (`src/ImageModule.ts:5`; `Image.tsx:95` reads `ImageModule.Image`) | Import throws | `Image`, `Image.prefetch` ([`packages/ui/src/lib/expo-image-backend.tsx:12-24`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/lib/expo-image-backend.tsx#L12-L24)) |
| expo-video | `ExpoVideo`, required at import (`src/ExpoVideo.ts:16`); `VideoPlayer.tsx:9,23` patch `VideoPlayer.prototype` at import | Import throws | `createVideoPlayer`, `useVideoPlayer`, `VideoView` ([`packages/tv/src/features/playback/`](https://github.com/kromatv/kroma/tree/1d270635adcc9f3a81283e04f06336845d8c5985/packages/tv/src/features/playback)) |
| expo-constants | `ExponentConstants`, optional | Warning; `expoConfig` null | `Constants.expoConfig?.extra?.buildInfo` (`tv-native/src/build-info.ts:5`) |

### Native API and minimum viable subset

| Module | Full native API (iOS definition) | Minimum for the TV app | Tizen implementation (unverified on a TV) |
|--------|----------------------------------|------------------------|---------------------------------------------|
| `ExpoFontLoader` | `getLoadedFonts(): string[]` (sync); `loadAsync(alias, localUri)`; `loadFontFamilyAsync(alias, [{localUri, weight?, style?}])` (`expo-font/ios/FontLoaderModule.swift:9-60`). JS downloads the file through expo-asset first (`src/FontLoader.ts:30-50`) | `getLoadedFonts`, `loadAsync`: `KIT_FONTS` is a name-to-file map ([`packages/ui/src/assets/fonts.ts:6-9`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/assets/fonts.ts#L6-L9)), which takes the `loadAsync` path | `modules/ExpoFontLoaderModule.cpp`: `getLoadedFonts`, `isLoaded`, `loadAsync`, `loadFontFamilyAsync`. Each font gets a directory of its own, keyed by alias and content, passed to `evas_font_path_global_append` (`Evas_Common.h:3374`) on the main thread; text then finds the font by its **family name** inside the file. A variable font with a `wght` axis goes in as static instances, one per hundred of its range, and the files are remembered per alias for letter spacing (`efl-rendering.md`, section 5 and Features). The work runs off the main loop. An alias that differs from the family name does not reach the text layer |
| `FileSystem` | Constants `documentDirectory`, `cacheDirectory` (file URIs, `ios/FileSystemModule.swift:194-200`), `bundleDirectory`; properties `totalDiskSpace`, `availableDiskSpace`; `info`, `downloadFileAsync`, event `downloadProgress`; classes `FileSystemFile` (`validatePath`, `exists`, `uri`, `size`, `md5`, `type`, times, `text`/`textSync`, `base64`, `bytes`, `write`/`writeSync`, `create`, `delete`, `copy`, `move`, `rename`, `info`, `open`, `digest`), `FileSystemDirectory` (`validatePath`, `exists`, `uri`, `size`, `info`, `create`, `delete`, `copy`, `move`, `rename`, `listAsRecords`, `createFile`, `createDirectory`), `FileSystemFileHandle`, upload and download tasks, `FileSystemWatcher` (`:184-607`) | The constants; `FileSystemFile` and `FileSystemDirectory` with constructor(uri), `validatePath`, `uri`, `exists`, `create`, `delete`, `write`, `writeSync`, `text`. Needs SharedObject classes | `modules/filesystem/`: the constants, `info`, disk space, `downloadFileAsync` (no progress events), and both classes with every member in the minimum plus `copy`, `move`, `rename`, `bytes`, `base64`, `md5`, `listAsRecords`, `createFile`, `createDirectory`. Paths from `app_get_data_path`, `app_get_cache_path`, `app_get_resource_path` (`appfw/app_common.h:314,326,337`, Tizen 9.0 rootstrap); "files stored in the application's cache directory can be removed by setting application or platform while the application is running" (`:321-322`). No file handle, watcher, upload or download task |
| `ExpoDevice` | Constants `isDevice`, `brand`, `manufacturer`, `modelId`, `modelName`, `deviceYearClass`, `totalMemory`, `osName`, `osVersion`, `osBuildId`, `osInternalBuildId`, `deviceName`, `deviceType`, `supportedCpuArchitectures` (`expo-device/ios/DeviceModule.swift:9-63`); Android adds `designName`, `productName`, `platformApiLevel`, `osBuildFingerprint`; async `getDeviceTypeAsync`, `getUptimeAsync`, `getMaxMemoryAsync`, `isRootedExperimentalAsync`, `isSideLoadingEnabledAsync`, `getPlatformFeaturesAsync`, `hasPlatformFeatureAsync` (`src/Device.ts:243-369`) | Every constant (null is allowed), `deviceType` 4 (`TV` in `src/Device.types.ts:8-24`) | `modules/ExpoDeviceModule.cpp`: every constant (`brand` and `manufacturer` "Samsung", `deviceType` 4, the rest from `DeviceProfile`, `sysconf` and `uname`) and the async functions. `deviceName` reads `SYSTEM_SETTINGS_KEY_DEVICE_NAME` (`system/system_settings.h:77,258`) through `dlopen`, so neither the link line nor the simulator changes; the model name when that fails |
| `ExpoKeepAwake` | `activate(tag): bool`, `deactivate(tag): bool`, `isActivated()` (`ios/KeepAwakeModule.swift:9-27`); optional `isAvailableAsync`, `addListenerForTag` (`src/index.ts:12,35,114`) | `activate`, `deactivate` over a set of tags | `modules/ExpoKeepAwakeModule.cpp`: a tag set on the main thread; `elm_win_noblank_set(window, !tags.empty())`. See "Keep awake" below |
| `ExpoBlur` views | See section 2 | `ExpoBlurView` drawing a translucent tint and ignoring `blurTargetId`; `ExpoBlurTargetView` as a plain container | `modules/blur/ExpoBlurViews.cpp`: an Evas snapshot of what is drawn behind the view under a `blur` filter, clipped to the view and its corners, and Android's overlay color above it (`TintStyle.kt`), from `intensity`, `tint` and `blurReductionFactor`; `blurTargetId` and `blurMethod` are not needed, since a snapshot sees every Evas object below it (`efl-rendering.md`, "Backdrop blur"). Without Evas filters, the tint alone. An empty `ExpoBlur` module answers the adapter's lookup |
| `ExpoImage` | `prefetch(urls, cachePolicy, headers)`, `clearMemoryCache`, `clearDiskCache`, `getCachePathAsync`, `configureCache`, `generateBlurhashAsync`, `generateThumbhashAsync`, `loadAsync`, `writeToCacheAsync`, `readFromCacheAsync`, class `Image`, event `imageLoaded`, and the view (`ios/ImageModule.swift:16-330`) | Module with `prefetch` and the two `clear` functions; view with `source`, `contentFit`, `transition`, `cachePolicy`, `recyclingKey`, `onLoad` (width, height), `onError` | `modules/image/`: `prefetch`, `clearMemoryCache` (a no-op that resolves true), `clearDiskCache`, `getCachePathAsync`; the view takes the first `source`, downloads it off the main thread into `<cache>/ExpoImage/<md5 of cacheKey or uri>`, has Evas decode it on its preload thread while hidden (at about the drawn size when the image is twice it or more and its loader reads a load size, which WebP's does not, efl-rendering.md section 6), fits it by `contentFit` (`cover`, `contain`, `fill`, `none`, `scale-down`, centered), fades in over `transition` ms once decoded and sends `onLoadStart`, `onLoad` (once decoded), `onError`, `onDisplay`. No placeholder, `contentPosition`, `tintColor`, headers or animated images |
| `ExpoVideo` | `isPictureInPictureSupported`, cache functions; class `VideoPlayer(source, useSynchronousReplace, builderOptions)` with about 30 properties, `play`, `pause`, `replace`, `replaceAsync`, `seekBy`, `replay`, `generateThumbnailsAsync`; events `statusChange`, `playingChange`, `playbackRateChange`, `volumeChange`, `mutedChange`, `playToEnd`, `timeUpdate`, `sourceChange`, track changes, `sourceLoad` (`ios/VideoModule.swift:8-392`; `src/VideoPlayerEvents.types.ts:17-90`) | `isPictureInPictureSupported` (false); `VideoPlayer` with `play`, `pause`, `replace`, `replaceAsync`, `release`, `currentTime` (read and write), `duration`, `volume`, `muted`, `loop`, `playbackRate`, `playing`, `status`, `bufferedPosition`, `bufferOptions`, `timeUpdateEventInterval`, `audioTrack`, `availableAudioTracks`, `videoTrack`; events `statusChange` `{status, oldStatus?, error?}`, `playingChange` `{isPlaying}`, `timeUpdate` `{currentTime, currentLiveTimestamp, currentOffsetFromLive, bufferedPosition}`, `playToEnd`; the view with `player`, `contentFit` | `modules/video/`: every member in the minimum and the rest of the class, on capi-media-player, run on a Samsung TV; see `tizen-media-playback.md` section 7 |
| `ExponentConstants` | Section 3 | `manifest`, `executionEnvironment: "bare"`, `deviceName`, `sessionId`, `systemFonts: []` | `modules/ExponentConstantsModule.cpp` reads `<res>/app.config`, written at package time by `src/package/app-config-file.ts` with expo-constants' own call, `getConfig(root, {isPublicConfig: true, skipSDKVersionRequirement: true})` (`expo-constants/scripts/getAppConfig.js`) |
| `ExpoAsset` | `downloadAsync(url, md5Hash or null, type)`: a file URL resolves as is; a remote one is cached as `ExponentAsset-<md5>.<type>`, re-downloaded when the md5 does not match. "do NOT change the function signature" (`expo-asset/ios/AssetModule.swift:20-47`) | Provided | `modules/ExpoAssetModule.cpp`, off the JavaScript thread, keyed as on iOS |

`File.write` is an AsyncFunction and `writeSync` the synchronous one in SDK 58
(`FileSystemModule.swift:382,386`). `tv-native/src/lib/storage.ts:36` writes then
reads `exists` on the next line without awaiting, on every platform. **src** On
Tizen `write` runs on a background thread too, so that `exists` reads false the
first time (verified in simulator, below): the store falls through to no file on
a first launch until it calls `writeSync` or awaits.

### Keep awake

| Fact | Source | Tag |
|------|--------|-----|
| "The screensaver is used to prevent screen burn when the TV screen displays a still image or the user provides no input for some time"; "You must disable the screensaver during activities when the user is watching the TV without giving input, such as video playback or image slideshows"; "To be published on Samsung Apps TV, your application must enable and disable the screensaver appropriately" | [Setting Screensaver](https://developer.samsung.com/tv/develop/guides/fundamentals/setting-screensaver) | **doc** |
| "If "Auto Protection Time" is switched off in the TV settings, enabling and disabling the screensaver using the AppCommon API has no effect" | same page | **doc** |
| Samsung documents the switch for web apps only: `webapis.appcommon.setScreenSaver(SCREEN_SAVER_OFF = 0 or SCREEN_SAVER_ON = 1)`, since 2.3, no privilege listed; no TizenFX or native C equivalent is listed in its API references | [AppCommon API](https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/appcommon-api.html) | **doc** |
| EFL's window property for exactly this case: "a way to request the display on which the window is shown does not blank, go to screensaver or otherwise hide or obscure the window. It is intended for uses such as media playback on a television"; it "may have no effect if the window is iconified/minimized or hidden" | `elementary-1/elm_win_legacy.h:1943-1957` (`elm_win_noblank_set`, since 1.11), Tizen 9.0 rootstrap | **doc** |
| `device_power_request_lock(POWER_LOCK_DISPLAY, 0)` keeps the display "ON(Normal)" until released and needs `http://tizen.org/privilege/display` | `device/power.h:41-55,64,88` | **doc** |
| Elementary stores a window's object on its Ecore_Evas under the data key `elm_win`; `ecore_evas_ecore_evas_list_get` and `ecore_evas_data_get` are public (`Ecore_Evas.h:3258,2237`) | the key is not documented; found in the simulator, where `elm_win_noblank_get` reads back true | **inf** |

`ExpoKeepAwake` uses `elm_win_noblank_set`: no privilege, already linked, and the
EFL contract names the TV case. Whether a retail TV's screensaver or Auto
Protection Time honours it is not known: record it on hardware. The power lock
is the fallback if it does not.

## 5. Third-party modules with Tizen support

| Fact | Tag |
|------|-----|
| `platforms` accepts "`android`, `apple` (or use the more granular `ios` / `macos` / `tvos`), `web` and `devtools`"; the only per-platform sections are `apple` and `android` ([module config](https://docs.expo.dev/modules/module-config/)) | **doc** |
| Custom platforms are not mentioned there or on the [autolinking page](https://docs.expo.dev/modules/autolinking/), whose `resolve --platform` takes `apple` or `android` | **doc** |
| Per-platform options are `expo.autolinking.android`, `.ios`, `.apple` ([autolinking](https://docs.expo.dev/modules/autolinking/), Configuration) | **doc** |
| For an unknown name, `supportsPlatform` is `platforms.includes(name)` (`expo-modules-autolinking/src/ExpoModuleConfig.ts:54-71`), and `findModulesAsync` filters with it (`autolinking/findModules.ts:31`) | **src** |
| The options loader reads `expo.autolinking[<any platform>]` and merges it (`commands/autolinkingOptions.ts:83-105`) | **src** |
| `resolveModulesAsync` and `generateModulesProviderAsync` throw for an unknown platform (`platforms/index.ts:32-52`, `autolinking/generatePackageList.ts:16-31`) | **src** |
| `SupportedPlatform` accepts any string (`src/types.ts:10`); `@expo/platforms`' `PlatformName` is closed: android, ios, web, tvos, macos (`build/index.d.ts:12`) | **src** |
| `ExpoModuleConfig` keeps the parsed JSON as `rawConfig` (`ExpoModuleConfig.ts:48`), so a `tizen` section survives | **inf** from the constructor |

So, without changing Expo (**inf**, from the table): a package declares
`"platforms": [..., "tizen"]` and a `tizen` section; `rn-tizen` calls the app's
own `findModulesAsync({ appRoot, autolinkingOptions: { ...options, platform: "tizen" } })`
and resolves the results itself, since `resolve` cannot.

```json
{
  "platforms": ["apple", "android", "tizen"],
  "tizen": {
    "modules": ["ExpoKeepAwake"],
    "cmake": "tizen/CMakeLists.txt",
    "target": "expo_keep_awake_tizen",
    "privileges": ["http://tizen.org/privilege/display"]
  }
}
```

`rn-tizen` would `add_subdirectory` each `cmake`, link each `target` into
`libreact-native-tizen.so`, and generate one C++ file that calls every package's
registration function, the way Apple's generated `ExpoModulesProvider` lists
every module. Generating it avoids static initializers, which a linker drops from
a static library unless it is linked whole-archive. Implementations of Expo's own
packages (expo-video, expo-image) stay in this package, keyed by package name.
**inf**

### Config plugins and a `tizen` section

| Fact | Tag |
|------|-----|
| Mods are documented for Android and iOS only; `withDangerousMod` takes "A native platform (**android** or **ios**)" ([mods](https://docs.expo.dev/config-plugins/mods/), [dangerous mods](https://docs.expo.dev/config-plugins/dangerous-mods/)) | **doc** |
| `evalModsAsync` runs every key of `config.mods`, skipping platforms not in `platforms` (`@expo/config-plugins/build/plugins/mod-compiler.js:172-178`) | **src** |
| The first mod of a (platform, name) pair must be a provider, else `MISSING_PROVIDER` (`mod-compiler.js:200-208`) | **src** |
| `withBaseMod`, `withMod`, `evalModsAsync` and `compileModsAsync` are exported (`build/index.d.ts:24,29`); `ModPlatform` is typed `android` or `ios` (`Plugin.types.d.ts:89,179`) | **src** |
| `getConfig` applies plugins unless `skipPlugins` (`@expo/config/build/Config.js:221`); `rn-tizen` does not skip them (`src/app/app-project.ts:27-29`) | **src** |

Two roads (**inf**, from the rows above). Today a plugin can set
`config.tizen.privileges` and `rn-tizen` reads it, with no mod. For generated
files (`tizen-manifest.xml`, the shell project), `rn-tizen` can register base
providers with `withBaseMod(config, { platform: "tizen", mod: "manifest",
isProvider: true, action })` and run `evalModsAsync(config, { projectRoot,
platforms: ["tizen"] })`, with a cast on the platform type. Neither is documented,
so pin the `@expo/config-plugins` range they were tested on.

## Implications for react-native-tizen

### Gaps, in priority order

| # | Gap | Why | Needs |
|---|-----|-----|-------|
| P2 | `app.config` in the simulator | `src/simulator/stage-app.ts` stages no `app.config`, so `Constants.expoConfig` is null there; a package has it | Call `writeAppConfig` when staging |
| P3 | `reloadAppAsync` | Rejects with `ERR_UNAVAILABLE` | A host reload path |
| P3 | Streaming fetch | `HttpEngine` hands over the body whole, so `expo/fetch` streams one chunk and `start()` settles at the end of the transfer | Incremental callbacks in the engine |
| P4 | Linking third-party Tizen modules | Section 5 | Autolinking search, CMake, generated provider |
| Done | `ExpoDevice`, `ExpoFontLoader`, `ExpoKeepAwake`, `ExpoImage`, `FileSystem`, `ExponentConstants`, `ExpoFetchModule`, `ExpoBlur` views, `ExpoVideo` (`tizen-media-playback.md` section 7); shared objects, classes with accessors, async functions, events, Expo views and their methods | | `runtime/host/expo/` |

### Bugs in the current Expo runtime

| Where | Bug | Effect | Severity |
|-------|-----|--------|----------|
| Was | Fixed by |
|-----|----------|
| A no-op SharedObject releaser and no ids | `SharedObjectRegistry`: ids from 1 under a mutex, never reused; `release()` and the GC both delete; the releaser touches no runtime |
| `getViewConfig` returned `undefined` | The view registry answers it (section 2) |
| `asyncFunction` ran on the JavaScript thread | Bodies run on a background pool or the main loop and settle through the CallInvoker (`Invoke.h`) |
| ExpoAsset wrote in place, unchecked; ignored `md5Hash` on a hit; keyed by `std::hash`; fell back to `/tmp` | Temporary file then rename, checked; re-downloads on an MD5 mismatch; MD5 of the URL as on iOS. `/tmp` remains the last resort, now only when `app_get_cache_path` returns null |
| `reloadAppAsync` threw synchronously | Returns a rejected promise (P3 above) |
| `expoModulesCoreVersion` was a string; no `uuidv5`, `cacheDir`, `documentsDir` | Installed as the contract types them |
| `Expo.cmake` copied Apple's `NativeState.h` and left out `common/cpp/fabric` | Both fixed: `EventEmitter.h` takes the `jsi::NativeState` branch, as on Android |
| `expo/fetch` named imports threw | `ExpoFetchModule` on the host's `HttpEngine` (P3 above for streaming) |

Still open: registration by static initializer (`ExpoModuleRegistry.h`) works for
sources compiled into the library; **inf**: a module in a static library is dropped
unless linked whole-archive, which matters for P4.

### The C++ module-authoring API

It keeps the shape of Expo's Swift and Kotlin DSL, so a module reads the same on
every platform, and keeps `jsi` behind it (`ModuleDefinition.h`, `Invoke.h`,
`Convert.h`).

| Expo DSL | C++ | Runs on |
|----------|-----|---------|
| `Name`, `Constant` | `ModuleDefinition("ExpoDevice").constant("deviceName", [] { return name; })` | once, when JavaScript first reads the module |
| `Property` | `.property("totalDiskSpace", getter)` | JS thread, on every read |
| `Function` | `.function("name", lambda)`; arguments and result convert from the lambda's own types | JS thread |
| `AsyncFunction` | `.asyncFunction("name", lambda)`, or `(..., Queue::Main)` for Evas work; a last `Promise` parameter settles later, from any thread | a shared background pool, or the EFL main loop; settled through the CallInvoker |
| `OnStartObserving`, `OnStopObserving` | `.onStartObserving(f)`, `.onStopObserving(f)` | JS thread |
| events | `sendEvent("name", payload)`, `sendEvent(object, "name", payload)`, `Context::emit` | any thread; delivered on the JS thread |
| `Class`, `Constructor`, `Property`, `Function` | `.sharedClass(ClassDefinition<File>("FileSystemFile").constructor(f).property("exists", g).function("text", h))`; methods take `File &` first. `.property("muted", get, set)` adds a setter, `set(T &, value)` | as above |
| `View`, `Prop`, `Events`, `AsyncFunction` | `RN_TIZEN_EXPO_VIEW(ViewDefinition<ImageView>("ExpoImage").prop("source", &ImageView::setSource).events({"onLoad"}))`; `.asyncFunction("enterFullscreen", &VideoView::enterFullscreen)` for a method called through the view's ref | main thread |

Conversions: `bool`, numbers, `std::string`, `std::optional`, `std::vector`,
`std::map<std::string, T>`, `folly::dynamic`, `Bytes` (Uint8Array, or any typed
array or ArrayBuffer in), two-way `std::variant`, and `std::shared_ptr<T>` of a
SharedObject class, which accepts the JavaScript object or its numeric id and
returns the paired object or a new instance of `T`'s class. A thrown
`std::exception` becomes a JavaScript `Error`; a `CodedError` sets `code`.

```cpp
class ExpoKeepAwakeModule final : public rntizen::expo::Module {
 public:
  rntizen::expo::ModuleDefinition definition() override {
    return rntizen::expo::ModuleDefinition("ExpoKeepAwake")
        .asyncFunction("activate", [this](std::string tag) { return update(tag, true); }, Queue::Main)
        .asyncFunction("deactivate", [this](std::string tag) { return update(tag, false); }, Queue::Main);
  }
};

RN_TIZEN_EXPO_MODULE(ExpoKeepAwake, ExpoKeepAwakeModule);
```

What holds the pieces together:

- One `Context` per app (`Context.h`): the CallInvoker, the host's `ModuleContext`,
  the resource directory and the module instances. It is created by the
  `ExpoModulesCore` TurboModule, the one place the host hands out a CallInvoker.
- One `RuntimeState` per runtime, in the runtime's data, holding every JavaScript
  value native code needs later: module objects, classes, paired objects (weakly)
  and pending promises. It dies with the runtime, so no `jsi` value outlives it.
- The `SharedObjectRegistry` is process-wide, so a view on the main thread finds a
  shared object prop by id without a runtime.
- A view definition is static: it must exist before Fabric builds its component
  registry, which happens before any module object.

### Verified in simulator

`rn-tizen simulator`, Ubuntu 26.04 aarch64 with its EFL, 2026-11-02, from the
example's `src/screens/expo/` screens (`main.ts` there is their entry):

| What | Seen |
|------|------|
| `globalThis.expo` installed through the `ExpoModulesCore` TurboModule | every check below ran |
| `expoModulesCoreVersion`, `uuid.v4`, `uuid.v5` | `{"version":"58.0.23","major":58,...}`; v5 of `hello` in the DNS namespace is `9342d47a-1bab-5709-9869-c840b2eac501`, as Python's `uuid.uuid5` |
| `getViewConfig("ExpoImage")` and an unknown module | the props and `top*` events; null |
| Shared objects | a `File` gets id 2 (ids from 1), `release()` resets it to 0; `addListener` and `emit` on it deliver |
| Async functions | settle after the call returns; `File.write` leaves `exists` false until awaited |
| `reloadAppAsync` | rejects |
| `expo/fetch` | `text()`, `json()`, headers, `response.body` as a stream, abort rejecting |
| `ExpoDevice`, `ExponentConstants` | `deviceType` 4, the model, Tizen 9.0, memory, `aarch64`; `expoConfig.name` from a staged `app.config`; `executionEnvironment` `bare` |
| `ExpoKeepAwake` | finds the window through `elm_win` and `elm_win_noblank_get` reads back true |
| `FileSystem` | `Paths`, `Directory.create`, `writeSync` then `exists`, `text`, `textSync`, `size`, `md5`, append, `bytes`, `list`, `copy`, `moveSync`, `delete`, `Paths.info`, disk space |
| `ExpoFontLoader` through `ExpoAsset` | a TTF downloaded from a URL, then `loadAsync`; text with `fontFamily: 'Pacifico'` draws in it |
| `ExpoImage` | a remote JPEG with `cover`, `contain` and `fill`, `onLoad` with 600x400, `none` then `disk` on the next run, the fade; placed in canvas pixels and clipped by the view's own box, so it scales with the logical screen at 1280x720 |
| `ExpoBlur` | the tint over an image for `dark` and `light`, at 1920x1080 and 1280x720; the blur under it, rounded and square, `example/src/screens/rendering2` |
| `ExpoFontLoader` with a variable font | Hanken Grotesk from a URL written as nine weights, each measured within 2.5 points of Chrome |
| `FileSystem`, `ExponentConstants` paths | `Paths.document` and `Paths.cache` are the simulator's `app_get_data_path` and `app_get_cache_path` (the instance's `data/` and `cache/`), `app.config` is read from `app_get_resource_path` (the staged app) |

Not verified: anything on a TV; a font or image bundled as an app asset (the
screens use URLs); GC-driven release; `deviceName` from system settings, which the
simulator does not have.

### Open questions

- Whether Metro's transform cache keys on `EXPO_PUBLIC_USE_RN_FETCH`. A bundle
  transformed once without it could keep `expo/fetch` as the global. Check with a
  cleared cache.
- Whether a retail TV honours `elm_win_noblank_set` against its screensaver and
  Auto Protection Time, and whether `SYSTEM_SETTINGS_KEY_DEVICE_NAME` answers on a
  TV: record both on hardware.
- Whether `app.config` should travel inside the bundle rather than beside it, so
  `rn-tizen simulator run --bundle` and a package agree.
