#include "PlatformModuleRegistry.h"

#include <functional>
#include <map>
#include <mutex>

namespace rntizen {

namespace {

struct Registry {
  std::mutex mutex;
  std::map<std::string, PlatformModuleFactory, std::less<>> factories;
};

Registry &registry() {
  static Registry shared;
  return shared;
}

} // namespace

void registerPlatformModule(std::string name, PlatformModuleFactory factory) {
  auto &shared = registry();
  std::lock_guard lock(shared.mutex);
  shared.factories.insert_or_assign(std::move(name), std::move(factory));
}

std::optional<PlatformModuleFactory> findPlatformModule(const std::string &name) {
  auto &shared = registry();
  std::lock_guard lock(shared.mutex);
  auto found = shared.factories.find(name);
  if (found == shared.factories.end()) return std::nullopt;
  return found->second;
}

} // namespace rntizen
