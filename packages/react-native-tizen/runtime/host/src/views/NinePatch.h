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

/// A width per side, in pixels.
struct Edges {
  int left{0};
  int top{0};
  int right{0};
  int bottom{0};

  bool operator==(const Edges &) const = default;
  bool isZero() const;
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
  /// Each radius less the widths of the sides it joins, never below zero: the
  /// corners of the box inside a border of `widths`.
  Corners inset(const Edges &widths) const;
  /// How far the corners reach in from each side: the larger radius of that
  /// side's two corners.
  Edges extents() const;
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

/// A filled rounded rectangle, from the shared patch cache (`PatchCache.h`).
/// Main thread only.
const NinePatch &filledPatch(const Corners &corners);

/// The rounded rectangle minus the one inset by `widths`, from the shared
/// patch cache. Main thread only.
const NinePatch &ringPatch(const Corners &corners, const Edges &widths);

/// The rounded rectangle blurred as CSS blurs a box-shadow (a Gaussian whose
/// standard deviation is half of `blur`), with `shadowPadding(blur)` pixels of
/// fade outside the shape on every side. Not cached; any thread.
NinePatch makeShadowPatch(const Corners &corners, int blur);

int shadowPadding(int blur);

} // namespace rntizen
