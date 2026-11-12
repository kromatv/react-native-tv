# Native libraries on Tizen

How `rn-tizen` builds an app's third-party native code into
`libreact-native-tizen.so`, and how a library adds its own Tizen support.

## What `rn-tizen` does with an app's dependencies

Every runtime build (`rn-tizen run`, `package`, `simulator run`, `simulator
build`) walks the app's dependencies the way Expo's autolinking does:
`dependencies` and non-optional `peerDependencies` at every depth, and
`devDependencies` of the app only. The app's own `react-native` is left out. A
package with a `codegenConfig` in its package.json is a **native library**:

```json
"codegenConfig": { "name": "rnmaps", "type": "all", "jsSrcsDir": "src/specs" }
```

For each one, `rn-tizen`:

1. Runs React Native's codegen over `jsSrcsDir` and keeps the C++ output only.
   Specs are read as Android reads them: a `.android.ts` spec counts, a
   `.ios.ts` one does not, because a Tizen bundle falls back to Android's sources.
2. Compiles that output into the runtime and registers the descriptor of every
   component codegen writes a shadow node for, so those components lay out.
3. Adds the library's Tizen implementation, when there is one: the package's
   own (below), else one react-native-tizen bundles.

The "Native libraries" step of the build lists them, `maps (codegen only)` for a
library with no Tizen implementation. Such a library still lays out: its views
mount as empty views, a component declared `interfaceOnly` becomes an
unimplemented view, and its TurboModules are missing
(`TurboModuleRegistry.get` returns null, `getEnforcing` throws).

When two copies of a library share a codegen name, the one nearest the app wins
and the build prints a warning.

## Declaring Tizen support in a library

Add a `tizen` field beside `codegenConfig`:

```json
"tizen": { "cmake": "tizen/CMakeLists.txt" }
```

`rn-tizen` adds that CMakeLists.txt with `add_subdirectory` while it configures
the runtime. It creates no target of its own: it adds sources to the one
`rn-tizen` made for the library.

| Name | What it is |
|------|------------|
| `RN_TIZEN_LIBRARY_TARGET` | An OBJECT library linked into `libreact-native-tizen.so`. It already holds the codegen output, and its include path has React Native's headers, the codegen root (`<name>JSI.h`, `react/renderer/components/<name>/...`) and the host's `runtime/host/src` |
| `RN_TIZEN_LIBRARY_PACKAGE` | The npm name |
| `RN_TIZEN_LIBRARY_DIR` | The package's directory |
| `target_link_tizen_libraries(<target> <libs>...)` | Links Tizen native libraries such as `evas` or `ecore`, and fails on one outside the target Tizen version's public native API, or with no stand-in in the simulator |

```cmake
file(GLOB _maps_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${RN_TIZEN_LIBRARY_DIR}/common/cpp/react/renderer/components/rnmaps/*.cpp")
target_sources(${RN_TIZEN_LIBRARY_TARGET} PRIVATE ${_maps_sources})
target_include_directories(${RN_TIZEN_LIBRARY_TARGET} PUBLIC "${RN_TIZEN_LIBRARY_DIR}/common/cpp")
target_link_tizen_libraries(${RN_TIZEN_LIBRARY_TARGET} evas ecore)
```

The objects are linked into the library itself, so registrations made by static
initializers are kept. The host's headers are not a stable API yet: a library
built against one react-native-tizen version may need changes for the next.

## Declaring Tizen support in an Expo module

An Expo module, a dependency or one of the app's own local modules, declares
its Tizen implementation in `expo-module.config.json`: `tizen` in `platforms`
and the CMakeLists.txt in `tizen.cmake`. Expo's autolinking ignores both for
Android and Apple: a platform list is checked with `includes`
(`expo-modules-autolinking/src/ExpoModuleConfig.ts:54-71`).

```json
{
  "platforms": ["android", "apple", "tizen"],
  "android": { "modules": ["expo.modules.serverdiscovery.ServerDiscoveryModule"] },
  "apple": { "modules": ["ServerDiscoveryModule"] },
  "tizen": { "cmake": "tizen/CMakeLists.txt" }
}
```

`rn-tizen` finds them where Expo's autolinking does: in every dependency it
walks, and in each directory of the app's `expo.autolinking.nativeModulesDir`,
`modules` unless the app's package.json says otherwise (`tizen` overrides
first), as in `expo-modules-autolinking/src/commands/autolinkingOptions.ts:79-124,279-281`.
Each becomes a library as above, with no codegen: its target is named
`expo_<name>`, where `<name>` is the npm name or the local module's directory,
and its CMakeLists.txt adds the sources to `RN_TIZEN_LIBRARY_TARGET`. Every
library target also sees the host's Expo C++ API, so the module includes
`expo/ModuleDefinition.h` and `expo/ExpoModuleRegistry.h` and registers with
`RN_TIZEN_EXPO_MODULE` ([reference/expo-modules-contract.md](reference/expo-modules-contract.md),
"The C++ module-authoring API").

```cmake
file(GLOB _sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
target_sources(${RN_TIZEN_LIBRARY_TARGET} PRIVATE ${_sources})
```

A module with no Tizen implementation is left out, and its JavaScript gets
null from `requireOptionalNativeModule` or throws from `requireNativeModule`.

## Writing the implementation

Everything registers itself from its own file ([decisions.md](decisions.md)):

| The library has | Register it with | From |
|-----------------|------------------|------|
| A component codegen writes the shadow node of | nothing, `rn-tizen` registers it | |
| An `interfaceOnly` component, with its shadow node in the library's C++ | `RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::<Name>ComponentDescriptor)` | `fabric/ComponentDescriptorRegistry.h` |
| A native view | `RN_TIZEN_COMPONENT_VIEW("<Name>", View)`, `View` deriving from `rntizen::ComponentView` | `mounting/ComponentViewRegistry.h`, `views/ComponentView.h` |
| A TurboModule | `RN_TIZEN_PLATFORM_MODULE("<JS name>", Module)`, `Module` built from `(jsInvoker, ModuleContext)`, usually on codegen's `Native<Spec>CxxSpec<Module>` from `<name>JSI.h` | `modules/PlatformModuleRegistry.h` |

A view receives its codegen props class, `<Name>Props`, which derives from
`ViewProps`. Evas may only be touched on the main loop, and drawing follows
[reference/efl-rendering.md](reference/efl-rendering.md).

## Implementations react-native-tizen bundles

For a package without Tizen support of its own, react-native-tizen can ship one
in `runtime/host/src/<dir>`: a `library.json` naming the package and a
CMakeLists.txt written as above. It compiles only for an app that depends on the
package, and a package's own `tizen` field wins over it. Adding one is adding
that directory; nothing else lists it.

| Package | Directory |
|---------|-----------|
| react-native-svg | `runtime/host/src/svg` |

### react-native-svg

Each `<Svg>` paints its whole tree into one Evas VG object, clipped to the view.

| Works | Not drawn yet |
|-------|---------------|
| `Svg` with `viewBox` and `preserveAspectRatio`, `G`, `Path` (all of SVG's path data), `Circle`, `Ellipse`, `Rect` with `rx`/`ry`, `Line`, `Polyline`, `Polygon` | `Text`, `TSpan`, `TextPath`, `Image`, `Use`, `Symbol`, `ForeignObject`, filters |
| `fill`, `stroke`, `strokeWidth`, `strokeLinecap`, `strokeLinejoin`, `opacity`, `fillOpacity`, `strokeOpacity`, `currentColor` and `color`, `matrix` and `transform`, `display="none"`, inheritance from `G` | Gradients and patterns (a `url(#id)` paint draws nothing), `clipPath`, `mask`, markers, `fillRule="evenodd"`, `strokeDasharray`, `strokeMiterlimit` |
| Lengths in user units, `px`, `pt`, `pc`, `mm`, `cm`, `in` and `%` | `em`, `ex` |

`opacity` on a `G` multiplies into each shape, so overlapping shapes inside a
translucent group show through each other. The TurboModules (`getBBox`,
`isPointInFill`, `toDataURL`...) are missing.

## Limits

- A bundled implementation does not check the package's version.
- Codegen output that does not compile fails the runtime build, and an app
  cannot leave a library out yet.
- A library cannot declare Tizen privileges.
- An Expo module is found by its `expo-module.config.json` only; Expo's
  `searchPaths`, `exclude` and `include` autolinking options are not read.
