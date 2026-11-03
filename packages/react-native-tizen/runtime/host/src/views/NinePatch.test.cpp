#include "NinePatch.h"

#include "Premultiplied.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>

namespace rntizen {

namespace {

std::uint32_t alphaAt(const NinePatch &patch, int x, int y) {
  return patch.pixels[static_cast<std::size_t>(y * patch.width + x)] >> 24;
}

constexpr Corners kMixed{
    .topLeft = {4, 4},
    .topRight = {10, 6},
    .bottomRight = {0, 0},
    .bottomLeft = {2, 12},
};

} // namespace

TEST_CASE("Corners extents take the larger radius of each side's two corners") {
  CHECK(kMixed.extents() == Edges{.left = 4, .top = 6, .right = 10, .bottom = 12});
}

TEST_CASE("Corners inset subtracts the adjacent border widths and stops at zero") {
  const auto inner = kMixed.inset({.left = 3, .top = 5, .right = 20, .bottom = 1});

  CHECK(inner.topLeft == CornerRadius{1, 0});
  CHECK(inner.topRight == CornerRadius{0, 1});
  CHECK(inner.bottomRight == CornerRadius{0, 0});
  CHECK(inner.bottomLeft == CornerRadius{0, 11});
}

TEST_CASE("Corners grown leaves square corners square") {
  const auto grown = kMixed.grown(3);

  CHECK(grown.topLeft == CornerRadius{7, 7});
  CHECK(grown.topRight == CornerRadius{13, 9});
  CHECK(grown.bottomRight == CornerRadius{0, 0});
  CHECK(kMixed.grown(-20).topLeft == CornerRadius{0, 0});
  CHECK(Corners{}.isSquare());
  CHECK_FALSE(kMixed.isSquare());
}

TEST_CASE("filledPatch keeps the corners as borders around one stretched pixel") {
  const auto &patch = filledPatch(kMixed);

  CHECK(patch.border == Edges{.left = 4, .top = 6, .right = 10, .bottom = 12});
  CHECK(patch.width == 4 + 10 + 1);
  CHECK(patch.height == 6 + 12 + 1);
  CHECK(patch.pixels.size() == static_cast<std::size_t>(patch.width * patch.height));
  CHECK(alphaAt(patch, 4, 6) == 255);
  CHECK(alphaAt(patch, 0, 0) == 0);
  CHECK(alphaAt(patch, patch.width - 1, patch.height - 1) == 255);
}

TEST_CASE("filledPatch pixels are white premultiplied by their coverage") {
  const auto &patch = filledPatch(kMixed);

  for (const auto pixel : patch.pixels) {
    const auto alpha = pixel >> 24;
    CHECK(pixel == (alpha << 24 | alpha << 16 | alpha << 8 | alpha));
  }
}

TEST_CASE("ringPatch is clear inside the hole and opaque across the band") {
  const Corners round{.topLeft = {6, 6}, .topRight = {6, 6}, .bottomRight = {6, 6}, .bottomLeft = {6, 6}};
  const auto &patch = ringPatch(round, {.left = 2, .top = 2, .right = 2, .bottom = 2});

  CHECK(patch.border == Edges{.left = 6, .top = 6, .right = 6, .bottom = 6});
  CHECK(alphaAt(patch, 6, 6) == 0);
  CHECK(alphaAt(patch, 6, 0) == 255);
  CHECK(alphaAt(patch, 6, 1) == 255);
  CHECK(alphaAt(patch, 6, 2) == 0);
}

TEST_CASE("ringPatch borders reach the widest border width") {
  const auto &patch = ringPatch(Corners{}, {.left = 1, .top = 9, .right = 3, .bottom = 0});

  CHECK(patch.border == Edges{.left = 1, .top = 9, .right = 3, .bottom = 0});
  CHECK(alphaAt(patch, 1, 8) == 255);
  CHECK(alphaAt(patch, 1, 9) == 0);
}

TEST_CASE("shadowPadding is one and a half blurs, rounded up") {
  CHECK(shadowPadding(0) == 0);
  CHECK(shadowPadding(-4) == 0);
  CHECK(shadowPadding(1) == 2);
  CHECK(shadowPadding(10) == 15);
}

TEST_CASE("makeShadowPatch pads the shape on every side and fades towards the edge") {
  const auto patch = makeShadowPatch(Corners{}, 10);
  const auto pad = shadowPadding(10);

  CHECK(patch.border == Edges{.left = 2 * pad, .top = 2 * pad, .right = 2 * pad, .bottom = 2 * pad});
  CHECK(patch.width == 4 * pad + 1);
  const auto middle = patch.height / 2;
  CHECK(alphaAt(patch, 0, middle) < alphaAt(patch, pad, middle));
  CHECK(alphaAt(patch, pad, middle) < alphaAt(patch, 2 * pad, middle));
  CHECK(alphaAt(patch, 2 * pad, middle) > 200);
}

TEST_CASE("makeShadowPatch without blur is the sharp shape") {
  const auto patch = makeShadowPatch(Corners{}, 0);

  CHECK(patch.width == 1);
  CHECK(patch.height == 1);
  CHECK(alphaAt(patch, 0, 0) == 255);
}

TEST_CASE("pixelOf packs premultiplied channels as ARGB and alphaPixel as white") {
  CHECK(pixelOf({0.5f, 0.25f, 0, 0.5f}) == 0x80804000u);
  CHECK(pixelOf({2, -1, 0, 1}) == 0xffff0000u);
  CHECK(alphaPixel(1) == 0xffffffffu);
  CHECK(alphaPixel(0) == 0u);
}

} // namespace rntizen
