#pragma once

namespace rntizen {

/// Overrides React Native's feature flags once per process, before any is
/// read: the OSS Stable set Android and iOS ship, plus
/// `cxxNativeAnimatedEnabled`, which serves `NativeAnimatedModule` from React
/// Native's C++ driver (`animated/NativeAnimatedModule.cpp`).
void overrideFeatureFlagsOnce();

} // namespace rntizen
