#include "TurboModuleProvider.h"

#include "../logging/Log.h"
#include "PlatformModuleRegistry.h"

#include <ReactCommon/CxxTurboModuleUtils.h>
#include <react/nativemodule/defaults/DefaultTurboModules.h>

namespace rntizen {

TurboModuleProvider::TurboModuleProvider(ModuleContext context, std::shared_ptr<facebook::react::CallInvoker> jsInvoker)
    : context_(std::move(context)), jsInvoker_(std::move(jsInvoker)) {}

std::shared_ptr<facebook::react::TurboModule> TurboModuleProvider::get(const std::string &name) {
  std::lock_guard lock(mutex_);
  if (auto existing = modules_.find(name); existing != modules_.end()) return existing->second;
  auto module = create(name);
  if (module) {
    modules_.emplace(name, module);
  } else if (missing_.insert(name).second) {
    log(LogLevel::Debug, "No native module named " + name);
  }
  return module;
}

std::shared_ptr<facebook::react::TurboModule> TurboModuleProvider::create(const std::string &name) const {
  if (auto factory = findPlatformModule(name)) return (*factory)(jsInvoker_, context_);
  if (auto module = facebook::react::DefaultTurboModules::getTurboModule(name, jsInvoker_)) return module;
  auto &registered = facebook::react::globalExportedCxxTurboModuleMap();
  if (auto factory = registered.find(name); factory != registered.end()) return factory->second(jsInvoker_);
  return nullptr;
}

} // namespace rntizen
