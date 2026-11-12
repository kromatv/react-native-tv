# Expo modules contract

What Expo SDK 58 needs from a platform before its JavaScript runs, and what
react-native-webos gives it today. Read it before touching `runtime/host/expo/`.

This file came with the port from react-native-tizen. An observation dated
before 2026-11-11 was made with that package, in its simulator or on a Samsung
TV, and only the names were changed here: it holds for webOS as far as the
code is shared. What ran on an LG TV is in
[hardware-verification.md](hardware-verification.md).

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

| Member | Read by | If missing | webOS today |
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
webOS registers that TurboModule (`runtime/host/expo/ExpoModulesCoreModule.cpp`):
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
| 7 | Fabric finds a ComponentDescriptor by that name. On webOS an unknown name gets `UnimplementedNativeViewComponentDescriptor` and draws empty | `runtime/host/src/fabric/ComponentDescriptorRegistry.cpp:52-62` **src** |
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

How webOS hosts Expo views (`runtime/host/expo/views/`):

1. `runtime/cmake/Expo.cmake` compiles `common/cpp/fabric/*.cpp`.
2. A view is an `ExpoViewType` (module, view, prop setters, event names, factory),
   written with `ViewDefinition<View>` and registered with `RN_WEBOS_EXPO_VIEW`.
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

expo-blur resolves its Android file on webOS: the Metro resolver keeps a
`.webos.*` match and otherwise resolves for `android`
(`src/metro/with-webos.ts:10,86-98`), so `NativeBlurModule.android.tsx:3,5` asks for
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
| `unstable_createBlobData(bytes)` | Returns a React Native BlobModule id, for `response.blob()`. webOS stores the bytes in the host's `BlobStore`, which `BlobModule` reads (verified in simulator) | `createBlob.ts:18`; `ExpoFetchModule.swift:27-38` **src** |

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
its package fails and the app does not start. webOS registers every module below.

| Package | Native module | Missing on webOS | TV app usage |
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

| Module | Full native API (iOS definition) | Minimum for the TV app | webOS implementation (what ran on a TV: `hardware-verification.md`) |
|--------|----------------------------------|------------------------|---------------------------------------------|
| `ExpoFontLoader` | `getLoadedFonts(): string[]` (sync); `loadAsync(alias, localUri)`; `loadFontFamilyAsync(alias, [{localUri, weight?, style?}])` (`expo-font/ios/FontLoaderModule.swift:9-60`). JS downloads the file through expo-asset first (`src/FontLoader.ts:30-50`) | `getLoadedFonts`, `loadAsync`: `KIT_FONTS` is a name-to-file map ([`packages/ui/src/assets/fonts.ts:6-9`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/ui/src/assets/fonts.ts#L6-L9)), which takes the `loadAsync` path | `modules/ExpoFontLoaderModule.cpp`: `getLoadedFonts`, `isLoaded`, `loadAsync`, `loadFontFamilyAsync`. Each file goes to `registerAppFont` (`runtime/host/src/text/TextEngine.cpp`): Skia reads it and the typeface is registered under the **alias** in the `TypefaceFontProvider` the text engine's font collection takes app fonts from (`skia-rendering.md`, section 6), so `fontFamily` finds the font by the name given to `loadAsync`. A variable font with a `wght` axis goes in as one instance per hundred of its range (`VariableFont.cpp`), since SkParagraph picks a typeface by the weight it reports. The work runs on the background pool, under the text engine's mutex. A missing file fails with `ERR_FONT_SOURCE`, one Skia cannot read with `ERR_FONT_FILE` |
| `FileSystem` | Constants `documentDirectory`, `cacheDirectory` (file URIs, `ios/FileSystemModule.swift:194-200`), `bundleDirectory`; properties `totalDiskSpace`, `availableDiskSpace`; `info`, `downloadFileAsync`, event `downloadProgress`; classes `FileSystemFile` (`validatePath`, `exists`, `uri`, `size`, `md5`, `type`, times, `text`/`textSync`, `base64`, `bytes`, `write`/`writeSync`, `create`, `delete`, `copy`, `move`, `rename`, `info`, `open`, `digest`), `FileSystemDirectory` (`validatePath`, `exists`, `uri`, `size`, `info`, `create`, `delete`, `copy`, `move`, `rename`, `listAsRecords`, `createFile`, `createDirectory`), `FileSystemFileHandle`, upload and download tasks, `FileSystemWatcher` (`:184-607`) | The constants; `FileSystemFile` and `FileSystemDirectory` with constructor(uri), `validatePath`, `uri`, `exists`, `create`, `delete`, `write`, `writeSync`, `text`. Needs SharedObject classes | `modules/filesystem/`: the constants, `info`, disk space, `downloadFileAsync` (no progress events), and both classes with every member in the minimum plus `copy`, `move`, `rename`, `bytes`, `base64`, `md5`, `listAsRecords`, `createFile`, `createDirectory`. Paths from `appPaths()` (`runtime/host/src/platform/AppPaths.cpp`): `documentDirectory` and `cacheDirectory` are `appdata/data/` and `appdata/cache/` beside the executable, in the world-writable `appdata` the package ships, since the TV runs the app as a user that does not own its install directory (`webos-tv-platform.md`, section 3.1); `bundleDirectory` is `res/` beside it. When `appdata` cannot be written, both go under a private `mkdtemp` directory of the temporary one. No file handle, watcher, upload or download task |
| `ExpoDevice` | Constants `isDevice`, `brand`, `manufacturer`, `modelId`, `modelName`, `deviceYearClass`, `totalMemory`, `osName`, `osVersion`, `osBuildId`, `osInternalBuildId`, `deviceName`, `deviceType`, `supportedCpuArchitectures` (`expo-device/ios/DeviceModule.swift:9-63`); Android adds `designName`, `productName`, `platformApiLevel`, `osBuildFingerprint`; async `getDeviceTypeAsync`, `getUptimeAsync`, `getMaxMemoryAsync`, `isRootedExperimentalAsync`, `isSideLoadingEnabledAsync`, `getPlatformFeaturesAsync`, `hasPlatformFeatureAsync` (`src/Device.ts:243-369`) | Every constant (null is allowed), `deviceType` 4 (`TV` in `src/Device.types.ts:8-24`) | `modules/ExpoDeviceModule.cpp`: every constant (`brand` "LG", `manufacturer` "LG Electronics", `osName` "webOS", `deviceType` 4, the model, version and firmware from `DeviceProfile`, which asks the TV's `getSystemInfo` at start, the rest from `sysconf` and `uname`) and the async functions but `getMaxMemoryAsync`. `deviceName` is the model name: LG documents no setting for the name the viewer gave the TV (`webos-tv-platform.md`, section 4.3) |
| `ExpoKeepAwake` | `activate(tag): bool`, `deactivate(tag): bool`, `isActivated()` (`ios/KeepAwakeModule.swift:9-27`); optional `isAvailableAsync`, `addListenerForTag` (`src/index.ts:12,35,114`) | `activate`, `deactivate` over a set of tags | `modules/ExpoKeepAwakeModule.cpp`: a tag set on the main thread; `SDL_DisableScreenSaver()` while it holds a tag, `SDL_EnableScreenSaver()` once it is empty; `isActivated` and `isAvailableAsync` too. See "Keep awake" below |
| `ExpoBlur` views | See section 2 | `ExpoBlurView` drawing a translucent tint and ignoring `blurTargetId`; `ExpoBlurTargetView` as a plain container | `modules/blur/ExpoBlurViews.cpp`: what the frame holds behind the view, blurred by Skia in a backdrop layer cut to the view and its corners, and Android's overlay color above it (`TintStyle.kt`), from `intensity`, `tint` and `blurReductionFactor`; `blurTargetId` and `blurMethod` are not needed, since the layer reads everything drawn before it (`skia-rendering.md`, section 11). An empty `ExpoBlur` module answers the adapter's lookup |
| `ExpoImage` | `prefetch(urls, cachePolicy, headers)`, `clearMemoryCache`, `clearDiskCache`, `getCachePathAsync`, `configureCache`, `generateBlurhashAsync`, `generateThumbhashAsync`, `loadAsync`, `writeToCacheAsync`, `readFromCacheAsync`, class `Image`, event `imageLoaded`, and the view (`ios/ImageModule.swift:16-330`) | Module with `prefetch` and the two `clear` functions; view with `source`, `contentFit`, `transition`, `cachePolicy`, `recyclingKey`, `onLoad` (width, height), `onError` | `modules/image/`: `prefetch`, `clearMemoryCache` (forgets the decoded images kept in memory), `clearDiskCache`, `getCachePathAsync`; the view takes the first `source`, shows a decoded copy kept in memory at once (`cacheType` `memory`), and otherwise downloads it off the main thread into `<cache>/ExpoImage/<md5 of cacheKey or uri>` and decodes it on the host's image threads (at the drawn size when the image is twice it or more, `skia-rendering.md` section 11), fits it by `contentFit` (`cover`, `contain`, `fill`, `none`, `scale-down`, centered), fades in over `transition` ms once decoded and sends `onLoadStart`, `onLoad` (once decoded), `onError`, `onDisplay`. No placeholder, `contentPosition`, `tintColor`, headers or animated images |
| `ExpoVideo` | `isPictureInPictureSupported`, cache functions; class `VideoPlayer(source, useSynchronousReplace, builderOptions)` with about 30 properties, `play`, `pause`, `replace`, `replaceAsync`, `seekBy`, `replay`, `generateThumbnailsAsync`; events `statusChange`, `playingChange`, `playbackRateChange`, `volumeChange`, `mutedChange`, `playToEnd`, `timeUpdate`, `sourceChange`, track changes, `sourceLoad` (`ios/VideoModule.swift:8-392`; `src/VideoPlayerEvents.types.ts:17-90`) | `isPictureInPictureSupported` (false); `VideoPlayer` with `play`, `pause`, `replace`, `replaceAsync`, `release`, `currentTime` (read and write), `duration`, `volume`, `muted`, `loop`, `playbackRate`, `playing`, `status`, `bufferedPosition`, `bufferOptions`, `timeUpdateEventInterval`, `audioTrack`, `availableAudioTracks`, `videoTrack`; events `statusChange` `{status, oldStatus?, error?}`, `playingChange` `{isPlaying}`, `timeUpdate` `{currentTime, currentLiveTimestamp, currentOffsetFromLive, bufferedPosition}`, `playToEnd`; the view with `player`, `contentFit` | `modules/video/`: every member in the minimum and the rest of the class, on the TV's media pipeline, `com.webos.media` over the Luna bus (`LunaMediaPlayer`), or a simulated player where the bus does not answer; run on webOS 24 and 25 TVs (`hardware-verification.md`); see `webos-media-playback.md`, "What is built" |
| `ExponentConstants` | Section 3 | `manifest`, `executionEnvironment: "bare"`, `deviceName`, `sessionId`, `systemFonts: []` | `modules/ExponentConstantsModule.cpp` reads `<res>/app.config`, written at package time by `src/package/app-config-file.ts` with expo-constants' own call, `getConfig(root, {isPublicConfig: true, skipSDKVersionRequirement: true})` (`expo-constants/scripts/getAppConfig.js`) |
| `ExpoAsset` | `downloadAsync(url, md5Hash or null, type)`: a file URL resolves as is; a remote one is cached as `ExponentAsset-<md5>.<type>`, re-downloaded when the md5 does not match. "do NOT change the function signature" (`expo-asset/ios/AssetModule.swift:20-47`) | Provided | `modules/ExpoAssetModule.cpp`, off the JavaScript thread, keyed as on iOS |

`File.write` is an AsyncFunction and `writeSync` the synchronous one in SDK 58
(`FileSystemModule.swift:382,386`). `tv-native/src/lib/storage.ts:36` writes then
reads `exists` on the next line without awaiting, on every platform. **src** On
webOS `write` runs on a background thread too, so that `exists` reads false the
first time (verified in simulator, below): the store falls through to no file on
a first launch until it calls `writeSync` or awaits.

### Keep awake

| Fact | Source | Tag |
|------|--------|-----|
| LG documents no call that holds the screen saver off: it stays off during full screen video, and `screenSaverProperties` in `appinfo.json` stretches its timeout to 30 minutes on OLED TVs with webOS TV 5.0 or higher | `webos-tv-platform.md`, sections 2 and 4.3 | **doc** |
| Before the saver starts, the TV asks each client subscribed to `luna://com.webos.service.tvpower/power/registerScreenSaverRequest` with `{"subscribe":true,"clientName":"<id>.wakelock"}`. The answer goes to `.../responseScreenSaverRequest` with the request's `timestamp`: `"ack":false` refuses the saver, `"ack":true` lets it start. An event without a `timestamp` is not a request. Homebrew apps and SDL's webOS port do this; it is not LG's word | `webos-tv-platform.md`, section 4.3 | **src** |
| `SDL_DisableScreenSaver` and `SDL_EnableScreenSaver` flip a switch and, on webOS, nothing else: the TV's compositor lists no idle inhibit protocol. SDL answers the TV's requests from that switch only when it registers the app itself, and it disables the screen saver when video starts unless `SDL_VIDEO_ALLOW_SCREENSAVER` is set | `webos-window-input.md`, section 3 | **src** |
| The host registers the app itself (`SDL_HINT_WEBOS_REGISTER_APP` off) and leaves the screen saver allowed at start (`SDL_HINT_VIDEO_ALLOW_SCREENSAVER`). It subscribes as `<APPID>.wakelock` and answers each request with `ack` set to `SDL_ScreenSaverEnabled()` | `runtime/host/src/main.cpp`, `runtime/host/src/platform/ScreenSaver.cpp`; `webos-window-input.md`, section 15, Lifecycle | **src** |

`ExpoKeepAwake` flips SDL's switch, and the host refuses the TV's screen saver
requests while it is off. No ACG group is listed for the two `tvpower` calls
(`webos-tv-platform.md`, section 4.3). Whether a TV sends the request to the
host's own subscription, without SDL's registration, is not known
(`webos-window-input.md`, section 13): record it on hardware.

## 5. Third-party modules with webOS support

| Fact | Tag |
|------|-----|
| `platforms` accepts "`android`, `apple` (or use the more granular `ios` / `macos` / `tvos`), `web` and `devtools`"; the only per-platform sections are `apple` and `android` ([module config](https://docs.expo.dev/modules/module-config/)) | **doc** |
| Custom platforms are not mentioned there or on the [autolinking page](https://docs.expo.dev/modules/autolinking/), whose `resolve --platform` takes `apple` or `android` | **doc** |
| Per-platform options are `expo.autolinking.android`, `.ios`, `.apple` ([autolinking](https://docs.expo.dev/modules/autolinking/), Configuration) | **doc** |
| For an unknown name, `supportsPlatform` is `platforms.includes(name)` (`expo-modules-autolinking/src/ExpoModuleConfig.ts:54-71`), and `findModulesAsync` filters with it (`autolinking/findModules.ts:31`) | **src** |
| The options loader reads `expo.autolinking[<any platform>]` and merges it (`commands/autolinkingOptions.ts:83-105`) | **src** |
| `resolveModulesAsync` and `generateModulesProviderAsync` throw for an unknown platform (`platforms/index.ts:32-52`, `autolinking/generatePackageList.ts:16-31`) | **src** |
| `SupportedPlatform` accepts any string (`src/types.ts:10`); `@expo/platforms`' `PlatformName` is closed: android, ios, web, tvos, macos (`build/index.d.ts:12`) | **src** |
| `ExpoModuleConfig` keeps the parsed JSON as `rawConfig` (`ExpoModuleConfig.ts:48`), so a `webos` section survives | **inf** from the constructor |

So, without changing Expo (**inf**, from the table): a package declares
`"platforms": [..., "webos"]` and a `webos` section; `rn-webos` calls the app's
own `findModulesAsync({ appRoot, autolinkingOptions: { ...options, platform: "webos" } })`
and resolves the results itself, since `resolve` cannot.

```json
{
  "platforms": ["apple", "android", "webos"],
  "webos": {
    "modules": ["ExpoKeepAwake"],
    "cmake": "webos/CMakeLists.txt",
    "target": "expo_keep_awake_webos"
  }
}
```

`rn-webos` would `add_subdirectory` each `cmake`, link each `target` into the
`react-native-webos` executable, and generate one C++ file that calls every package's
registration function, the way Apple's generated `ExpoModulesProvider` lists
every module. Generating it avoids static initializers, which a linker drops from
a static library unless it is linked whole-archive. Implementations of Expo's own
packages (expo-video, expo-image) stay in this package, keyed by package name.
**inf**

### Config plugins and a `webos` section

| Fact | Tag |
|------|-----|
| Mods are documented for Android and iOS only; `withDangerousMod` takes "A native platform (**android** or **ios**)" ([mods](https://docs.expo.dev/config-plugins/mods/), [dangerous mods](https://docs.expo.dev/config-plugins/dangerous-mods/)) | **doc** |
| `evalModsAsync` runs every key of `config.mods`, skipping platforms not in `platforms` (`@expo/config-plugins/build/plugins/mod-compiler.js:172-178`) | **src** |
| The first mod of a (platform, name) pair must be a provider, else `MISSING_PROVIDER` (`mod-compiler.js:200-208`) | **src** |
| `withBaseMod`, `withMod`, `evalModsAsync` and `compileModsAsync` are exported (`build/index.d.ts:24,29`); `ModPlatform` is typed `android` or `ios` (`Plugin.types.d.ts:89,179`) | **src** |
| `getConfig` applies plugins unless `skipPlugins` (`@expo/config/build/Config.js:221`); `rn-webos` does not skip them (`src/app/app-project.ts:27-29`) | **src** |

Two roads (**inf**, from the rows above). With no mod, a plugin sets a field of
the config that `rn-webos` reads. Today `rn-webos` reads its webOS settings,
`permissions` (the ACG groups of `requiredACG` in `appinfo.json`) among them,
only from the props of this package's own entry in `expo.plugins`
(`src/config/webos-settings.ts`, `src/config/webos-config.ts`), not from a
`webos` key another plugin could set. For generated files (`appinfo.json`),
`rn-webos` can register base providers with `withBaseMod(config, { platform:
"webos", mod: "appinfo", isProvider: true, action })` and run
`evalModsAsync(config, { projectRoot, platforms: ["webos"] })`, with a cast on
the platform type. Neither is documented, so pin the `@expo/config-plugins`
range they were tested on.

## Implications for react-native-webos

### Gaps, in priority order

| # | Gap | Why | Needs |
|---|-----|-----|-------|
| P2 | `app.config` in the simulator | Fixed: `src/simulator/stage-app.ts` calls `writeAppConfig`, so `Constants.expoConfig` is the app's config there as in a package | Done |
| P3 | `reloadAppAsync` | Rejects with `ERR_UNAVAILABLE` | A host reload path |
| P3 | Streaming fetch | `HttpEngine` hands over the body whole, so `expo/fetch` streams one chunk and `start()` settles at the end of the transfer | Incremental callbacks in the engine |
| P4 | Linking third-party webOS modules | Section 5 | Autolinking search, CMake, generated provider |
| Done | `ExpoDevice`, `ExpoFontLoader`, `ExpoKeepAwake`, `ExpoImage`, `FileSystem`, `ExponentConstants`, `ExpoFetchModule`, `ExpoBlur` views, `ExpoVideo` (`webos-media-playback.md`, "What is built"); shared objects, classes with accessors, async functions, events, Expo views and their methods | | `runtime/host/expo/` |

### Bugs in the current Expo runtime

| Where | Bug | Effect | Severity |
|-------|-----|--------|----------|
| Was | Fixed by |
|-----|----------|
| A no-op SharedObject releaser and no ids | `SharedObjectRegistry`: ids from 1 under a mutex, never reused; `release()` and the GC both delete; the releaser touches no runtime |
| `getViewConfig` returned `undefined` | The view registry answers it (section 2) |
| `asyncFunction` ran on the JavaScript thread | Bodies run on a background pool or the main loop and settle through the CallInvoker (`Invoke.h`) |
| ExpoAsset wrote in place, unchecked; ignored `md5Hash` on a hit; keyed by `std::hash`; fell back to `/tmp` | Temporary file then rename, checked; re-downloads on an MD5 mismatch; MD5 of the URL as on iOS. When the package's `appdata` directory cannot be written (`runtime/host/src/platform/AppPaths.cpp`), the directory is one `mkdtemp` makes, private (0700) and unpredictable, under the temporary directory, instead of a fixed name there |
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
| `AsyncFunction` | `.asyncFunction("name", lambda)`, or `(..., Queue::Main)` for work on the window or the views; a last `Promise` parameter settles later, from any thread | a shared background pool, or the main loop; settled through the CallInvoker |
| `OnStartObserving`, `OnStopObserving` | `.onStartObserving(f)`, `.onStopObserving(f)` | JS thread |
| events | `sendEvent("name", payload)`, `sendEvent(object, "name", payload)`, `Context::emit` | any thread; delivered on the JS thread |
| `Class`, `Constructor`, `Property`, `Function` | `.sharedClass(ClassDefinition<File>("FileSystemFile").constructor(f).property("exists", g).function("text", h))`; methods take `File &` first. `.property("muted", get, set)` adds a setter, `set(T &, value)` | as above |
| `View`, `Prop`, `Events`, `AsyncFunction` | `RN_WEBOS_EXPO_VIEW(ViewDefinition<ImageView>("ExpoImage").prop("source", &ImageView::setSource).events({"onLoad"}))`; `.asyncFunction("enterFullscreen", &VideoView::enterFullscreen)` for a method called through the view's ref | main thread |

Conversions: `bool`, numbers, `std::string`, `std::optional`, `std::vector`,
`std::map<std::string, T>`, `folly::dynamic`, `Bytes` (Uint8Array, or any typed
array or ArrayBuffer in), two-way `std::variant`, and `std::shared_ptr<T>` of a
SharedObject class, which accepts the JavaScript object or its numeric id and
returns the paired object or a new instance of `T`'s class. A thrown
`std::exception` becomes a JavaScript `Error`; a `CodedError` sets `code`.

```cpp
class ExpoKeepAwakeModule final : public rnwebos::expo::Module {
 public:
  rnwebos::expo::ModuleDefinition definition() override {
    return rnwebos::expo::ModuleDefinition("ExpoKeepAwake")
        .asyncFunction("activate", [this](std::string tag) { return update(tag, true); }, Queue::Main)
        .asyncFunction("deactivate", [this](std::string tag) { return update(tag, false); }, Queue::Main);
  }
};

RN_WEBOS_EXPO_MODULE(ExpoKeepAwake, ExpoKeepAwakeModule);
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

react-native-tizen's `rn-tizen simulator`, Ubuntu 26.04 aarch64 with its EFL,
2026-11-02, from the example's `src/screens/expo/` screens (`main.ts` there is
their entry). The same screens passed on a webOS 25 TV on 2026-11-11
([hardware-verification.md](hardware-verification.md)):

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

Not verified: a font or image bundled as an app asset (the screens use URLs);
GC-driven release; `ExpoKeepAwake` against a TV's screen saver.

### Open questions

- Whether Metro's transform cache keys on `EXPO_PUBLIC_USE_RN_FETCH`. A bundle
  transformed once without it could keep `expo/fetch` as the global. Check with a
  cleared cache.
- Whether a TV sends its screen saver requests to the host's own subscription,
  without SDL's registration (`webos-window-input.md`, section 13), so that
  `ExpoKeepAwake` holds the saver off: record it on hardware.
- Whether `app.config` should travel inside the bundle rather than beside it, so
  `rn-webos simulator run --bundle` and a package agree.
