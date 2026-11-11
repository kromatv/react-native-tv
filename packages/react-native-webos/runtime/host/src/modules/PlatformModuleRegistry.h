#pragma once

#include "../registry/Registration.h"
#include "ModuleContext.h"

#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/TurboModule.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace rntizen {

/// Creates one of this platform's TurboModules for a runtime.
using PlatformModuleFactory = std::function<std::shared_ptr<facebook::react::TurboModule>(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)>;

void registerPlatformModule(std::string name, PlatformModuleFactory factory);
std::optional<PlatformModuleFactory> findPlatformModule(const std::string &name);

template <typename Module>
bool registerPlatformModuleType(const char *name) {
  registerPlatformModule(name, [](std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context) {
    return std::make_shared<Module>(std::move(jsInvoker), context);
  });
  return true;
}

} // namespace rntizen

/// Registers `Type`, constructed from (jsInvoker, ModuleContext), as the TurboModule `name`.
#define RN_TIZEN_PLATFORM_MODULE(name, Type) RN_TIZEN_REGISTRATION(::rntizen::registerPlatformModuleType<Type>(name))
