#include "../mounting/LogicalScreen.h"
#include "Components.test.h"

#include <doctest/doctest.h>

#include <folly/dynamic.h>

#include <string>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

using facebook::react::LayoutConstraints;
using facebook::react::Size;
using testing::laidOutSize;

constexpr LayoutConstraints kTwiceTheScreen{
    .minimumSize = {.width = 0, .height = 0},
    .maximumSize = {.width = 2 * kLogicalScreenSize.width, .height = 2 * kLogicalScreenSize.height}};

} // namespace

TEST_CASE("every core component has its own descriptor under its React name") {
  const testing::Components components;

  for (const auto &[viewName, componentName] : std::vector<std::pair<std::string, std::string>>{
           {"RootView", "RootView"},
           {"RCTView", "View"},
           {"RCTText", "Paragraph"},
           {"RCTVirtualText", "Text"},
           {"RCTRawText", "RawText"},
           {"RCTImageView", "Image"},
           {"RCTScrollView", "ScrollView"},
           {"RCTModalHostView", "ModalHostView"},
           {"RNCMaskedView", "RNCMaskedView"},
           {"RCTActivityIndicatorView", "ActivityIndicatorView"},
           {"AndroidTextInput", "AndroidTextInput"},
       }) {
    CAPTURE(viewName);
    CHECK(componentName == components.descriptor(viewName).getComponentName());
  }
}

TEST_CASE("a Modal lays out at the logical screen's size from its first layout") {
  testing::Components components;

  const auto size = laidOutSize(components.create("ModalHostView", folly::dynamic::object()), kTwiceTheScreen);

  CHECK(size == kLogicalScreenSize);
}

TEST_CASE("a masked view lays out as a plain view") {
  testing::Components components;
  constexpr Size kMask{.width = 30, .height = 40};
  const auto mask =
      components.create("RNCMaskedView", folly::dynamic::object("width", kMask.width)("height", kMask.height));

  const auto size = laidOutSize(mask);

  CHECK(size == kMask);
}

} // namespace rntizen
