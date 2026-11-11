# Native libraries on webOS

How `rn-webos` builds an app's third-party native code into the
`react-native-webos` executable, and how a library adds its own webOS support.

## What `rn-webos` does with an app's dependencies

Every runtime build (`rn-webos run`, `package`, `simulator run`, `simulator
build`) walks the app's dependencies the way Expo's autolinking does:
`dependencies` and non-optional `peerDependencies` at every depth, and
`devDependencies` of the app only. The app's own `react-native` is left out. A
package with a `codegenConfig` in its package.json is a **native library**:

```json
"codegenConfig": { "name": "rnmaps", "type": "all", "jsSrcsDir": "src/specs" }
```

For each one, `rn-webos`:

1. Runs React Native's codegen over `jsSrcsDir` and keeps the C++ output only.
   Specs are read as Android reads them: a `.android.ts` spec counts, a
   `.ios.ts` one does not, because a webOS bundle falls back to Android's sources.
2. Compiles that output into the runtime and registers the descriptor of every
   component codegen writes a shadow node for, so those components lay out.
3. Adds the library's webOS implementation, when there is one: the package's
   own (below), else one react-native-webos bundles.

The "Native libraries" step of the build lists them, `maps (codegen only)` for a
library with no webOS implementation. Such a library still lays out: its views
mount as empty views, a component declared `interfaceOnly` becomes an
unimplemented view, and its TurboModules are missing
(`TurboModuleRegistry.get` returns null, `getEnforcing` throws).

When two copies of a library share a codegen name, the one nearest the app wins
and the build prints a warning.

## Declaring webOS support in a library

Add a `webos` field beside `codegenConfig`:

```json
"webos": { "cmake": "webos/CMakeLists.txt" }
```

`rn-webos` adds that CMakeLists.txt with `add_subdirectory` while it configures
the runtime. It creates no target of its own: it adds sources to the one
`rn-webos` made for the library.

| Name | What it is |
|------|------------|
| `RN_WEBOS_LIBRARY_TARGET` | An OBJECT library linked into the `react-native-webos` executable. It already holds the codegen output, and its include path has React Native's headers, the codegen root (`<name>JSI.h`, `react/renderer/components/<name>/...`) and the host's `runtime/host/src` |
| `RN_WEBOS_LIBRARY_PACKAGE` | The npm name |
| `RN_WEBOS_LIBRARY_DIR` | The package's directory |

```cmake
file(GLOB _maps_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${RN_WEBOS_LIBRARY_DIR}/common/cpp/react/renderer/components/rnmaps/*.cpp")
target_sources(${RN_WEBOS_LIBRARY_TARGET} PRIVATE ${_maps_sources})
target_include_directories(${RN_WEBOS_LIBRARY_TARGET} PUBLIC "${RN_WEBOS_LIBRARY_DIR}/common/cpp")
```

A library with a native view also links `rn_webos_skia` and
`SDL3::SDL3-static`: `views/ComponentView.h` includes the headers of both.

```cmake
target_link_libraries(${RN_WEBOS_LIBRARY_TARGET} PUBLIC rn_webos_skia SDL3::SDL3-static)
```

The objects are linked into the executable itself, so registrations made by
static initializers are kept. The host's headers are not a stable API yet: a
library built against one react-native-webos version may need changes for the
next.

A library is compiled for 32-bit ARM against the SDK's glibc 2.12.2, older
than any TV's, so it runs on every release the package does. One that links a
library of the TV may only name those every release carries under one name:
the list is in
[reference/webos-native-toolchain.md](reference/webos-native-toolchain.md),
"Safe to link from the TV". The TV's curl, OpenSSL, ICU and libjpeg change
their names between releases: compile such a dependency in instead.

## Declaring webOS support in an Expo module

An Expo module, a dependency or one of the app's own local modules, declares
its webOS implementation in `expo-module.config.json`: `webos` in `platforms`
and the CMakeLists.txt in `webos.cmake`. Expo's autolinking ignores both for
Android and Apple: a platform list is checked with `includes`
(`expo-modules-autolinking/src/ExpoModuleConfig.ts:54-71`).

```json
{
  "platforms": ["android", "apple", "webos"],
  "android": { "modules": ["expo.modules.serverdiscovery.ServerDiscoveryModule"] },
  "apple": { "modules": ["ServerDiscoveryModule"] },
  "webos": { "cmake": "webos/CMakeLists.txt" }
}
```

`rn-webos` finds them where Expo's autolinking does: in every dependency it
walks, and in each directory of the app's `expo.autolinking.nativeModulesDir`,
`modules` unless the app's package.json says otherwise (`webos` overrides
first), as in `expo-modules-autolinking/src/commands/autolinkingOptions.ts:79-124,279-281`.
Each becomes a library as above, with no codegen: its target is named
`expo_<name>`, where `<name>` is the npm name or the local module's directory,
and its CMakeLists.txt adds the sources to `RN_WEBOS_LIBRARY_TARGET`. Every
library target also sees the host's Expo C++ API, so the module includes
`expo/ModuleDefinition.h` and `expo/ExpoModuleRegistry.h` and registers with
`RN_WEBOS_EXPO_MODULE` ([reference/expo-modules-contract.md](reference/expo-modules-contract.md),
"The C++ module-authoring API").

```cmake
file(GLOB _sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
target_sources(${RN_WEBOS_LIBRARY_TARGET} PRIVATE ${_sources})
```

A module with no webOS implementation is left out, and its JavaScript gets
null from `requireOptionalNativeModule` or throws from `requireNativeModule`.

## Writing the implementation

Everything registers itself from its own file ([decisions.md](decisions.md)):

| The library has | Register it with | From |
|-----------------|------------------|------|
| A component codegen writes the shadow node of | nothing, `rn-webos` registers it | |
| An `interfaceOnly` component, with its shadow node in the library's C++ | `RN_WEBOS_COMPONENT_DESCRIPTOR(facebook::react::<Name>ComponentDescriptor)` | `fabric/ComponentDescriptorRegistry.h` |
| A native view | `RN_WEBOS_COMPONENT_VIEW("<Name>", View)`, `View` deriving from `rnwebos::ComponentView` | `mounting/ComponentViewRegistry.h`, `views/ComponentView.h` |
| A TurboModule | `RN_WEBOS_PLATFORM_MODULE("<JS name>", Module)`, `Module` built from `(jsInvoker, ModuleContext)`, usually on codegen's `Native<Spec>CxxSpec<Module>` from `<name>JSI.h` | `modules/PlatformModuleRegistry.h` |

A view receives its codegen props class, `<Name>Props`, which derives from
`ViewProps`. Drawing follows
[reference/skia-rendering.md](reference/skia-rendering.md).

## Implementations react-native-webos bundles

For a package without webOS support of its own, react-native-webos can ship one
in `runtime/host/src/<dir>`: a `library.json` naming the package and a
CMakeLists.txt written as above. It compiles only for an app that depends on the
package, and a package's own `webos` field wins over it. Adding one is adding
that directory; nothing else lists it.

| Package | Directory |
|---------|-----------|
| react-native-svg | `runtime/host/src/svg` |

### react-native-svg

Each `<Svg>` draws the tree of its elements with Skia, clipped to the view. It
records the drawing once and plays it every frame until an element changes; a
still one is kept as an image, so an icon costs one textured rectangle a frame.

| Works | Not drawn yet |
|-------|---------------|
| `Svg` with `viewBox` and `preserveAspectRatio`, `G`, `Path` (all of SVG's path data), `Circle`, `Ellipse`, `Rect` with `rx`/`ry`, `Line`, `Polyline`, `Polygon`, `Defs`, `Use`, `Symbol` | `Image`, `Marker`, `ForeignObject`, `TextPath`, filters |
| `fill`, `stroke`, `strokeWidth`, `strokeLinecap`, `strokeLinejoin`, `strokeMiterlimit`, `strokeDasharray`, `strokeDashoffset`, `fillRule`, `opacity`, `fillOpacity`, `strokeOpacity`, `currentColor` and `color`, `matrix` and `transform`, `display="none"`, inheritance from `G` and `Svg` | `vectorEffect="non-scaling-stroke"`, `context-fill` and `context-stroke` |
| `LinearGradient` and `RadialGradient` (both units, `gradientTransform`, a focal point), `Pattern`, `ClipPath` with `clipRule`, `Mask` (luminance and alpha) | `spreadMethod`, which the package does not send: a gradient pads |
| `Text` and `TSpan`: `x`, `y`, `dx`, `dy`, `fontFamily`, `fontSize`, `fontWeight`, `fontStyle`, `textAnchor`, fills and strokes | Per-glyph positions and `rotate`, `letterSpacing`, `textLength`, text decoration, ligatures and bidirectional text, a fallback font for characters the family lacks, text as a clip |
| Lengths in user units, `px`, `pt`, `pc`, `mm`, `cm`, `in` and `%`; font sizes in `em` too | `ex` |

`opacity` on a `G` fades the group as one picture, as a browser does. `clipRule`
is react-native-svg's, not SVG's: it belongs to the element that is clipped, and
without it overlapping shapes of a `ClipPath` leave a hole, as on Android and
iOS; `clipRule="nonzero"` clips to their union. A `Use` without `width` and
`height` gives a `Symbol` the whole viewport. The TurboModules (`getBBox`,
`isPointInFill`, `toDataURL`...) are missing.

## Limits

- A bundled implementation does not check the package's version.
- Codegen output that does not compile fails the runtime build, and an app
  cannot leave a library out yet.
- A library cannot add ACG groups to `appinfo.json`: the app lists the ones
  its libraries' Luna calls need in the config plugin's `permissions` prop.
- An Expo module is found by its `expo-module.config.json` only; Expo's
  `searchPaths`, `exclude` and `include` autolinking options are not read.
