#include "NinePatch.h"

#include "AlphaBlur.h"
#include "PatchCache.h"
#include "Premultiplied.h"
#include "RoundedRect.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rntizen {

namespace {

NinePatch blankPatch(const Edges &border) {
  NinePatch patch{.width = border.left + border.right + 1, .height = border.top + border.bottom + 1, .border = border};
  patch.pixels.resize(static_cast<std::size_t>(patch.width) * static_cast<std::size_t>(patch.height));
  return patch;
}

template <typename Alpha>
void paint(NinePatch &patch, Alpha alphaAt) {
  for (int y = 0; y < patch.height; ++y) {
    for (int x = 0; x < patch.width; ++x) patch.pixels[static_cast<std::size_t>(y * patch.width + x)] = alphaPixel(alphaAt(x, y));
  }
}

NinePatch makeFilled(const Corners &corners) {
  auto patch = blankPatch(corners.extents());
  const RoundedRect shape{.width = static_cast<float>(patch.width), .height = static_cast<float>(patch.height), .corners = corners};
  paint(patch, [&shape](int x, int y) { return shape.coverage(x, y); });
  return patch;
}

NinePatch makeRing(const Corners &corners, const Edges &widths) {
  const auto extents = corners.extents();
  auto patch = blankPatch({
      .left = std::max(extents.left, widths.left),
      .top = std::max(extents.top, widths.top),
      .right = std::max(extents.right, widths.right),
      .bottom = std::max(extents.bottom, widths.bottom),
  });
  const auto w = static_cast<float>(patch.width);
  const auto h = static_cast<float>(patch.height);
  const RoundedRect outer{.width = w, .height = h, .corners = corners};
  const RoundedRect hole{
      .x = static_cast<float>(widths.left),
      .y = static_cast<float>(widths.top),
      .width = w - static_cast<float>(widths.left + widths.right),
      .height = h - static_cast<float>(widths.top + widths.bottom),
      .corners = corners.inset(widths),
  };
  paint(patch, [&outer, &hole](int x, int y) { return outer.coverage(x, y) - hole.coverage(x, y); });
  patch.hollow = true;
  return patch;
}

} // namespace

bool Corners::isSquare() const {
  return *this == Corners{};
}

Corners Corners::grown(int amount) const {
  auto grow = [amount](const CornerRadius &r) {
    if (r.x == 0 && r.y == 0) return r;
    return CornerRadius{std::max(0, r.x + amount), std::max(0, r.y + amount)};
  };
  return {grow(topLeft), grow(topRight), grow(bottomRight), grow(bottomLeft)};
}

Corners Corners::inset(const Edges &widths) const {
  auto inner = [](const CornerRadius &r, int dx, int dy) {
    return CornerRadius{std::max(0, r.x - dx), std::max(0, r.y - dy)};
  };
  return {
      .topLeft = inner(topLeft, widths.left, widths.top),
      .topRight = inner(topRight, widths.right, widths.top),
      .bottomRight = inner(bottomRight, widths.right, widths.bottom),
      .bottomLeft = inner(bottomLeft, widths.left, widths.bottom),
  };
}

Edges Corners::extents() const {
  return {
      .left = std::max(topLeft.x, bottomLeft.x),
      .top = std::max(topLeft.y, topRight.y),
      .right = std::max(topRight.x, bottomRight.x),
      .bottom = std::max(bottomLeft.y, bottomRight.y),
  };
}

bool Edges::isZero() const {
  return *this == Edges{};
}

int shadowPadding(int blur) {
  return blur <= 0 ? 0 : static_cast<int>(std::ceil(static_cast<float>(blur) * 1.5f));
}

const NinePatch &filledPatch(const Corners &corners) {
  return cachedPatch(patchKey(PatchKind::Filled, corners), [&corners] { return makeFilled(corners); });
}

const NinePatch &ringPatch(const Corners &corners, const Edges &widths) {
  auto key = patchKey(PatchKind::Ring, corners);
  appendEdges(key, widths);
  return cachedPatch(key, [&corners, &widths] { return makeRing(corners, widths); });
}

NinePatch makeShadowPatch(const Corners &corners, int blur) {
  const auto pad = shadowPadding(blur);
  const auto extents = corners.extents();
  auto patch = blankPatch({
      .left = 2 * pad + extents.left,
      .top = 2 * pad + extents.top,
      .right = 2 * pad + extents.right,
      .bottom = 2 * pad + extents.bottom,
  });
  const RoundedRect shape{
      .x = static_cast<float>(pad),
      .y = static_cast<float>(pad),
      .width = static_cast<float>(patch.width - 2 * pad),
      .height = static_cast<float>(patch.height - 2 * pad),
      .corners = corners,
  };
  std::vector<float> alpha(patch.pixels.size());
  for (int y = 0; y < patch.height; ++y) {
    shape.coverageRow(y, patch.width, alpha.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(patch.width));
  }
  if (blur > 0) alpha = blurredAlpha(std::move(alpha), patch.width, patch.height, pad, static_cast<float>(blur) / 2);
  const auto stride = patch.width;
  paint(patch, [&alpha, stride](int x, int y) { return alpha[static_cast<std::size_t>(y * stride + x)]; });
  return patch;
}

} // namespace rntizen
