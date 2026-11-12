#include "SharedObjectRegistry.h"

#include <doctest/doctest.h>

#include <memory>

namespace rntizen::expo {

namespace {

class Tracked final : public SharedObject {
 public:
  void sharedObjectDidRelease() override {
    ++releases;
  }

  int releases{0};
};

class Other final : public SharedObject {};

} // namespace

TEST_CASE("SharedObjectRegistry gives each object a new id and finds it by that id") {
  auto &registry = SharedObjectRegistry::shared();
  const auto first = std::make_shared<Tracked>();
  const auto second = std::make_shared<Tracked>();
  CHECK(first->sharedObjectId() == 0);
  const auto firstId = registry.add(first);
  const auto secondId = registry.add(second);
  CHECK(firstId > 0);
  CHECK(secondId > firstId);
  CHECK(first->sharedObjectId() == firstId);
  CHECK(registry.find(firstId) == first);
  CHECK(registry.find<Tracked>(secondId) == second);
  CHECK(registry.find<Other>(secondId) == nullptr);
  CHECK(registry.find(0) == nullptr);
  registry.release(firstId);
  registry.release(secondId);
}

TEST_CASE("SharedObjectRegistry releases an object once, resets its id and reports the id") {
  auto &registry = SharedObjectRegistry::shared();
  registry.takeReleased();
  const auto object = std::make_shared<Tracked>();
  const auto id = registry.add(object);
  registry.release(id);
  registry.release(id);
  CHECK(object->releases == 1);
  CHECK(object->sharedObjectId() == 0);
  CHECK(registry.find(id) == nullptr);
  CHECK(registry.takeReleased() == std::vector<SharedObjectId>{id});
  CHECK(registry.takeReleased().empty());
  CHECK(registry.add(object) > id);
  registry.release(object->sharedObjectId());
}

} // namespace rntizen::expo
