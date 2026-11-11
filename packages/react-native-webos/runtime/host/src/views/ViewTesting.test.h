#pragma once

#include "../render/RasterTarget.test.h"
#include "ComponentView.h"

#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/graphics/Color.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rnwebos::testing {

/// The layout of a view at (x, y) of its parent, in points, at `scale` pixels per point.
inline facebook::react::LayoutMetrics frameAt(
    facebook::react::Float x,
    facebook::react::Float y,
    facebook::react::Float width,
    facebook::react::Float height,
    facebook::react::Float scale = 1) {
  facebook::react::LayoutMetrics metrics{};
  metrics.frame = {.origin = {.x = x, .y = y}, .size = {.width = width, .height = height}};
  metrics.pointScaleFactor = scale;
  return metrics;
}

/// A plain view's props once `change` set what the test needs.
template <typename Change>
facebook::react::Props::Shared viewProps(Change &&change) {
  auto props = std::make_shared<facebook::react::ViewProps>();
  std::forward<Change>(change)(*props);
  return props;
}

inline facebook::react::Props::Shared background(facebook::react::SharedColor color) {
  return viewProps([&color](facebook::react::ViewProps &props) { props.backgroundColor = color; });
}

/// Gives `view` its props and its layout, as mounting does.
inline void place(
    ComponentView &view,
    const facebook::react::Props::Shared &props,
    const facebook::react::LayoutMetrics &metrics) {
  view.updateProps(props);
  view.updateLayout(metrics);
}

/// A view that records what it is asked to draw, in order.
class RecordingView final : public ComponentView {
 public:
  RecordingView(Scene &scene, facebook::react::Tag tag, std::vector<std::string> &drawn)
      : ComponentView(scene, tag), drawn_(drawn) {}

 protected:
  void drawContent(SkCanvas &, float) override {
    drawn_.push_back("content " + std::to_string(tag()));
  }
  void drawOverChildren(SkCanvas &) override {
    drawn_.push_back("over " + std::to_string(tag()));
  }

 private:
  std::vector<std::string> &drawn_;
};

/// A 400x300 scene with a root view over it, drawn on a transparent target.
struct Screen {
  static constexpr PixelSize kSize{.width = 400, .height = 300};

  Scene scene{kSize};
  RasterTarget target{kSize};
  ComponentView root{scene, 1};

  Screen() {
    place(root, viewProps([](facebook::react::ViewProps &) {}), frameAt(0, 0, kSize.width, kSize.height));
    root.mountAsRoot(Scene::kSurfaceOrder);
  }

  void draw() {
    scene.setTransparent(true);
    scene.drawInto(target.canvas());
  }
};

} // namespace rnwebos::testing
