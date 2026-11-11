#pragma once

#include "ComponentView.h"

#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/graphics/Color.h>

#include <memory>
#include <utility>

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

} // namespace rnwebos::testing
