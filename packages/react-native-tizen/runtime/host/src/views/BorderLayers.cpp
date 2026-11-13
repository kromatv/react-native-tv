#include "BorderLayers.h"

#include "EvasColor.h"

#include <algorithm>
#include <array>

namespace rntizen {

namespace {

using facebook::react::SharedColor;

struct SideStyle {
  Side side;
  int width;
  const SharedColor &color;
};

float alphaOf(const SharedColor &color) {
  return facebook::react::colorComponentsFromColor(color).alpha;
}

void addSide(std::vector<BorderLayer> &layers, Side side, const SharedColor &color) {
  const auto same = std::ranges::find(layers, color, &BorderLayer::color);
  if (same == layers.end()) {
    layers.push_back({.part = {.sides = {side}}, .color = color});
  } else {
    same->part.sides = same->part.sides.with({side});
  }
}

void runUnderOpaqueLayers(std::vector<BorderLayer> &layers) {
  std::ranges::stable_sort(layers, {}, [](const BorderLayer &layer) { return alphaOf(layer.color); });
  SideSet opaqueAbove;
  for (auto layer = layers.rbegin(); layer != layers.rend(); ++layer) {
    layer->part.under = opaqueAbove;
    if (alphaOf(layer->color) >= 1) opaqueAbove = opaqueAbove.with(layer->part.sides);
  }
}

} // namespace

std::vector<BorderLayer> borderLayers(const Edges &widths, const facebook::react::BorderColors &colors) {
  using enum Side;
  const std::array styles{
      SideStyle{Top, widths.top, colors.top},
      SideStyle{Right, widths.right, colors.right},
      SideStyle{Bottom, widths.bottom, colors.bottom},
      SideStyle{Left, widths.left, colors.left},
  };
  std::vector<BorderLayer> layers;
  bool everySideShows = true;
  for (const auto &[side, width, color] : styles) {
    if (width <= 0) continue;
    if (isVisible(color)) {
      addSide(layers, side, color);
    } else {
      everySideShows = false;
    }
  }
  if (layers.size() == 1 && everySideShows) {
    layers.front().part = {};
  } else {
    runUnderOpaqueLayers(layers);
  }
  return layers;
}

} // namespace rntizen
