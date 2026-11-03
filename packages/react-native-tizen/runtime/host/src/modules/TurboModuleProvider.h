#pragma once

#include "ModuleContext.h"

#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/TurboModule.h>

#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

namespace rntizen {

/// Resolves a TurboModule by the name JavaScript asks for: this platform's own
/// modules first, then React Native's C++ defaults, then any C++ module a
/// library registered with `registerCxxModuleToGlobalModuleMap`. Each module is
/// created once; a name nothing answers is logged once and resolves to null.
class TurboModuleProvider final {
 public:
  TurboModuleProvider(ModuleContext context, std::shared_ptr<facebook::react::CallInvoker> jsInvoker);

  std::shared_ptr<facebook::react::TurboModule> get(const std::string &name);

 private:
  std::shared_ptr<facebook::react::TurboModule> create(const std::string &name) const;

  const ModuleContext context_;
  const std::shared_ptr<facebook::react::CallInvoker> jsInvoker_;
  std::mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<facebook::react::TurboModule>> modules_;
  std::set<std::string> missing_;
};

} // namespace rntizen
