#include "ComponentDescriptorRegistry.h"

#include <doctest/doctest.h>

#include <react/renderer/components/view/ViewComponentDescriptor.h>
#include <react/renderer/componentregistry/ComponentDescriptorProvider.h>

namespace rntizen {

TEST_CASE("hasComponent knows a registered component by its React name too") {
  registerComponentDescriptor(
      facebook::react::concreteComponentDescriptorProvider<facebook::react::ViewComponentDescriptor>());

  CHECK(hasComponent("View"));
  CHECK(hasComponent("RCTView"));
  CHECK_FALSE(hasComponent("NoSuchComponent"));
}

} // namespace rntizen
