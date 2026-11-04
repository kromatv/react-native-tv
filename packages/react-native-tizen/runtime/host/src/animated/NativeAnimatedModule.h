#pragma once

#include "../modules/ModuleContext.h"

#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/TurboModule.h>

#include <memory>

namespace rntizen {

/// `NativeAnimatedModule` served by React Native's C++ animated driver, its
/// frames from an AnimationFrameLoop; null while `cxxNativeAnimatedEnabled` is
/// off, which leaves JavaScript's driver in charge.
std::shared_ptr<facebook::react::TurboModule> createNativeAnimatedModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context);

} // namespace rntizen
