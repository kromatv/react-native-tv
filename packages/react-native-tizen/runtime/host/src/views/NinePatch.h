#pragma once

#include <cstdint>
#include <vector>

namespace rntizen {

/// One corner's radii in pixels; unequal radii make an elliptical corner.
struct CornerRadius {
  int x{0};
  int y{0};

  bool operator==(const CornerRadius &) const = default;
};

/// The corners of a rounded rectangle, in pixels.
struct Corners {
  CornerRadius topLeft;
  CornerRadius topRight;
  CornerRadius bottomRight;
  CornerRadius bottomLeft;

  bool operator==(const Corners &) const = default;
  bool isSquare() const;
  /// Every radius grown by `amount`, the way CSS grows a shadow's or an
  /// outline's corners; a square corner stays square.
  Corners grown(int amount) const;
};

/// A width per side, in pixels.
struct Edges {
  int left{0};
  int top{0};
  int right{0};
  int bottom{0};

  bool operator==(const Edges &) const = default;
  bool isZero() const;
};

/// An image whose `border` keeps its corners unscaled while the middle row and
/// column stretch (Evas's image border, the nine-patch of efl-rendering.md),
/// in premultiplied ARGB. The box patches below are white with a shape in
/// their alpha, so the object color tints them into a rounded background, a
/// ring or a shadow at any size; a gradient patch carries its own colors.
struct NinePatch {
  int width{0};
  int height{0};
  Edges border;
  std::vector<std::uint32_t> pixels;
};

/// A filled rounded rectangle. Patches are cached; a returned reference stays
/// valid until the next call. Main thread only.
const NinePatch &filledPatch(const Corners &corners);

/// The rounded rectangle minus the one inset by `widths`.
const NinePatch &ringPatch(const Corners &corners, const Edges &widths);

/// The rounded rectangle blurred as CSS blurs a box-shadow (a Gaussian whose
/// standard deviation is half of `blur`), with `shadowPadding(blur)` pixels of
/// fade outside the shape on every side. Not cached; any thread.
NinePatch makeShadowPatch(const Corners &corners, int blur);

int shadowPadding(int blur);

/// How much of the pixel at (`x`, `y`) a `width` x `height` rectangle with
/// `corners` covers, from 0 to 1.
float roundedCoverage(const Corners &corners, int width, int height, int x, int y);

/// Whether the point (`x`, `y`) lies in the `width` x `height` rectangle at
/// (`left`, `top`) with `corners`.
bool insideRounded(const Corners &corners, float left, float top, float width, float height, float x, float y);

} // namespace rntizen
