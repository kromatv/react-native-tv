#include "ComponentDescriptorRegistry.h"

#include "Components.test.h"

#include <doctest/doctest.h>

#include <react/renderer/components/view/ViewComponentDescriptor.h>
#include <react/renderer/componentregistry/ComponentDescriptorProvider.h>

#include <folly/dynamic.h>

#include <string>

namespace rntizen {

TEST_CASE("hasComponent knows a registered component by its React name too") {
  registerComponentDescriptor(
      facebook::react::concreteComponentDescriptorProvider<facebook::react::ViewComponentDescriptor>());

  CHECK(hasComponent("View"));
  CHECK(hasComponent("RCTView"));
  CHECK_FALSE(hasComponent("NoSuchComponent"));
}

TEST_CASE("the registry answers a component no descriptor knows with a plain view under its own name") {
  testing::Components components;
  constexpr facebook::react::Size kBox{.width = 30, .height = 40};

  const auto node =
      components.create("NoSuchComponent", folly::dynamic::object("width", kBox.width)("height", kBox.height));

  CHECK(std::string("NoSuchComponent") == node->getComponentName());
  CHECK(testing::laidOutSize(node) == kBox);
}

} // namespace rntizen
