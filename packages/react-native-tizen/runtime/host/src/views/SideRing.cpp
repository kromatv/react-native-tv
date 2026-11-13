#include "SideRing.h"

#include "PatchCache.h"
#include "Premultiplied.h"
#include "RoundedRect.h"

#include <algorithm>

namespace rntizen {

namespace {

// Evas's GL engine blends a stretched piece with the pixels beside it (efl-rendering.md, section 4):
// a pixel of straight border on each corner piece keeps a join that ends at its edge out of the blend.
constexpr int kStraightPixels = 1;

NinePatch paint(const Corners &corners, const Edges &widths, const RingPart &part) {
  const auto extents = corners.extents();
  const Edges border{
      .left = std::max(extents.left, widths.left) + kStraightPixels,
      .top = std::max(extents.top, widths.top) + kStraightPixels,
      .right = std::max(extents.right, widths.right) + kStraightPixels,
      .bottom = std::max(extents.bottom, widths.bottom) + kStraightPixels,
  };
  NinePatch patch{.width = border.left + border.right + 1, .height = border.top + border.bottom + 1, .border = border, .hollow = true};
  patch.pixels.resize(static_cast<std::size_t>(patch.width) * static_cast<std::size_t>(patch.height));
  const auto ring = borderRing(corners, widths, patch.width, patch.height);
  const SideJoins joins{.widths = widths, .reach = border, .width = patch.width, .height = patch.height};
  const auto inRing = [&ring](float x, float y) { return ring.outer.contains(x, y) && !ring.hole.contains(x, y); };
  for (int y = 0; y < patch.height; ++y) {
    for (int x = 0; x < patch.width; ++x) {
      if (ring.outer.coverage(x, y) <= ring.hole.coverage(x, y)) continue;
      patch.pixels[static_cast<std::size_t>(y * patch.width + x)] = alphaPixel(partCoverage(joins, part, x, y, inRing));
    }
  }
  return patch;
}

} // namespace

const NinePatch &sideRingPatch(const Corners &corners, const Edges &widths, const RingPart &part) {
  auto key = patchKey(PatchKind::SideRing, corners);
  appendEdges(key, widths);
  key.insert(key.end(), {static_cast<float>(part.sides.bits()), static_cast<float>(part.under.bits())});
  return cachedPatch(key, [&corners, &widths, &part] { return paint(corners, widths, part); });
}

} // namespace rntizen
