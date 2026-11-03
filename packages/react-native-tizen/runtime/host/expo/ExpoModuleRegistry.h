#pragma once

#include "registry/Registration.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rntizen::expo {

class Module;

/// Creates one Expo module for an app.
using ModuleFactory = std::function<std::shared_ptr<Module>()>;

/// Every Expo module implemented for Tizen, by the name JavaScript passes to
/// `requireNativeModule`. A module registers itself once, before React Native
/// starts, with RN_TIZEN_EXPO_MODULE.
class ExpoModuleRegistry final {
 public:
  static ExpoModuleRegistry &shared();

  void add(std::string name, ModuleFactory factory);

  /// A new instance of the module `name`, or null when none is registered.
  std::shared_ptr<Module> create(const std::string &name) const;

  std::vector<std::string> names() const;

 private:
  mutable std::mutex mutex_;
  std::map<std::string, ModuleFactory, std::less<>> factories_;
};

template <typename Type>
bool registerExpoModuleType(const char *name) {
  ExpoModuleRegistry::shared().add(name, [] { return std::make_shared<Type>(); });
  return true;
}

} // namespace rntizen::expo

/// Registers `Type`, a default-constructible rntizen::expo::Module, as the
/// Expo module `name` when the library loads.
#define RN_TIZEN_EXPO_MODULE(name, Type) RN_TIZEN_REGISTRATION(::rntizen::expo::registerExpoModuleType<Type>(#name))
