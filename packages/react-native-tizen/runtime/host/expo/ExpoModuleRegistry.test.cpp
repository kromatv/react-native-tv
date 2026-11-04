#include "ExpoModuleRegistry.h"

#include "ModuleDefinition.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <memory>
#include <string>

namespace rntizen::expo {

namespace {

class FirstRegistryModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("RegistryTestModule");
  }
};

class SecondRegistryModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("RegistryTestModule");
  }
};

} // namespace

TEST_CASE("ExpoModuleRegistry creates a new instance of a registered module each time, and nothing for another name") {
  auto &registry = ExpoModuleRegistry::shared();
  registry.add("RegistryTestModule", [] { return std::make_shared<FirstRegistryModule>(); });
  const auto first = registry.create("RegistryTestModule");
  CHECK(std::dynamic_pointer_cast<FirstRegistryModule>(first) != nullptr);
  CHECK(registry.create("RegistryTestModule") != first);
  CHECK(registry.create("RegistryTestModuleThatIsNot") == nullptr);
  const auto names = registry.names();
  CHECK(std::ranges::count(names, "RegistryTestModule") == 1);
  CHECK(std::ranges::is_sorted(names));
}

TEST_CASE("ExpoModuleRegistry lets a later registration of a name replace the first") {
  auto &registry = ExpoModuleRegistry::shared();
  registry.add("RegistryReplacedModule", [] { return std::make_shared<FirstRegistryModule>(); });
  CHECK(registerExpoModuleType<SecondRegistryModule>("RegistryReplacedModule"));
  CHECK(std::dynamic_pointer_cast<SecondRegistryModule>(registry.create("RegistryReplacedModule")) != nullptr);
}

} // namespace rntizen::expo
