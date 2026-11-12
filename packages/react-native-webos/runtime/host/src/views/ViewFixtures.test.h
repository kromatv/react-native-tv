#pragma once

#include "ComponentView.h"

#include <react/renderer/components/view/ViewProps.h>

#include <memory>

namespace rnwebos::testing {

/// Layout for a view at (`x`, `y`) of its parent, `width` x `height` points,
/// one pixel a point.
inline facebook::react::LayoutMetrics frameOf(
    facebook::react::Float x,
    facebook::react::Float y,
    facebook::react::Float width,
    facebook::react::Float height) {
  facebook::react::LayoutMetrics metrics;
  metrics.frame = {.origin = {.x = x, .y = y}, .size = {.width = width, .height = height}};
  return metrics;
}

/// Props of type `Props`, default but for what `change` sets.
template <typename Props = facebook::react::ViewProps, typename Change>
std::shared_ptr<const Props> propsWith(Change change) {
  auto props = std::make_shared<Props>();
  change(*props);
  return props;
}

/// View props with nothing but a background color.
inline facebook::react::Props::Shared filled(int red, int green, int blue, int alpha = 255) {
  return propsWith([&](facebook::react::ViewProps &props) {
    props.backgroundColor = facebook::react::colorFromRGBA(red, green, blue, alpha);
  });
}

/// Gives `view` its props and its layout, as mounting does.
inline void place(ComponentView &view, const facebook::react::Props::Shared &props, const facebook::react::LayoutMetrics &layout) {
  view.updateProps(props);
  view.updateLayout(layout);
}

/// A surface's root view across `scene`, mounted as its lowest layer.
inline std::unique_ptr<ComponentView> rootOf(Scene &scene) {
  auto root = std::make_unique<ComponentView>(scene, 1);
  const auto size = scene.size();
  place(*root, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, size.width, size.height));
  root->mountAsRoot(Scene::kSurfaceOrder);
  return root;
}

} // namespace rnwebos::testing
