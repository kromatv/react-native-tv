#include "AnimatedViewProps.h"

#include "FabricSurface.test.h"

#include <doctest/doctest.h>
#include <react/renderer/components/view/ViewProps.h>

namespace rntizen {

namespace {

using testing::FabricSurface;

const facebook::react::ViewProps &viewProps(const ComponentView &view) {
  return static_cast<const facebook::react::ViewProps &>(*view.props());
}

} // namespace

TEST_CASE("applyAnimatedProps sets what the animated driver computed on the mounted view, keeping its other props") {
  FabricSurface surface;
  surface.mountView(2, "{collapsable: false, testID: 'card', opacity: 1, width: 200, height: 100}");
  const auto *card = surface.mountingManager().findView(2);
  REQUIRE(card != nullptr);

  applyAnimatedProps(
      surface.mountingManager(),
      surface.scheduler(),
      surface.contextContainer(),
      FabricSurface::kSurfaceId,
      2,
      folly::dynamic::object("opacity", 0.5));

  CHECK(viewProps(*card).opacity == doctest::Approx(0.5));
  CHECK(viewProps(*card).testId == "card");
}

TEST_CASE("applyAnimatedProps skips a view that is not mounted and the surface's root") {
  FabricSurface surface;
  auto &mounting = surface.mountingManager();
  const auto *root = mounting.findView(FabricSurface::kSurfaceId);
  const auto apply = [&surface, &mounting](facebook::react::Tag tag) {
    applyAnimatedProps(
        mounting, surface.scheduler(), surface.contextContainer(), FabricSurface::kSurfaceId, tag, folly::dynamic::object("opacity", 0));
  };

  const auto unmountedRootProps = root->props();
  apply(FabricSurface::kSurfaceId);
  CHECK(root->props() == unmountedRootProps);
  surface.mountView(2, "{collapsable: false, width: 200, height: 100}");
  const auto mountedRootProps = root->props();
  apply(FabricSurface::kSurfaceId);
  apply(42);

  CHECK(root->props() == mountedRootProps);
  CHECK(mounting.findView(42) == nullptr);
  CHECK(viewProps(*mounting.findView(2)).opacity == 1);
}

} // namespace rntizen
