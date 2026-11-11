#include "ExpoModuleRegistry.h"

#include "ModuleDefinition.h"

namespace rntizen::expo {

ExpoModuleRegistry &ExpoModuleRegistry::shared() {
  static ExpoModuleRegistry registry;
  return registry;
}

void ExpoModuleRegistry::add(std::string name, ModuleFactory factory) {
  std::lock_guard lock(mutex_);
  factories_.insert_or_assign(std::move(name), std::move(factory));
}

std::shared_ptr<Module> ExpoModuleRegistry::create(const std::string &name) const {
  ModuleFactory factory;
  {
    std::lock_guard lock(mutex_);
    auto found = factories_.find(name);
    if (found == factories_.end()) return nullptr;
    factory = found->second;
  }
  return factory();
}

std::vector<std::string> ExpoModuleRegistry::names() const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> names;
  names.reserve(factories_.size());
  for (const auto &[name, factory] : factories_) names.push_back(name);
  return names;
}

} // namespace rntizen::expo
