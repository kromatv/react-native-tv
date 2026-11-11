#include "MountingManager.h"

#include <doctest/doctest.h>

namespace rnwebos {

TEST_CASE("MountingManager drops a command for a view it has not mounted") {
  Scene scene(PixelSize{100, 100});
  MountingManager manager(scene);

  CHECK_NOTHROW(manager.dispatchCommand(42, "focus", folly::dynamic::array()));
  CHECK(manager.findView(42) == nullptr);
}

TEST_CASE("MountingManager mounts a surface's root as a layer of the scene, and removes it with the surface") {
  Scene scene(PixelSize{100, 100});
  MountingManager manager(scene);

  manager.startSurface(1, Scene::kSurfaceOrder);
  const auto *root = manager.findView(1);
  REQUIRE(root != nullptr);
  CHECK(root->tag() == 1);
  CHECK_FALSE(manager.componentHandle(1).has_value());

  manager.stopSurface(1);

  CHECK(manager.findView(1) == nullptr);
}

} // namespace rnwebos
