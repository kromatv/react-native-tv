#pragma once

#include <jsi/jsi.h>

#include <memory>

namespace rntizen::expo {

class Context;

/// Installs `globalThis.expo` through the `ExpoModulesCore` TurboModule, which
/// holds the app's Expo Context. Run it while the runtime initializes, after
/// the TurboModule binding and before the bundle loads.
void installExpoRuntime(facebook::jsi::Runtime &runtime);

/// Installs `globalThis.expo` the way expo-modules-core's Android and iOS
/// runtimes do: the EventEmitter, NativeModule, SharedObject and SharedRef
/// classes, `expo.modules` and the helpers its JavaScript calls. Does nothing
/// when it is already installed.
void installExpoGlobals(facebook::jsi::Runtime &runtime, std::shared_ptr<Context> context);

} // namespace rntizen::expo
