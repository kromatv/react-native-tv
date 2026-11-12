#pragma once

namespace rnwebos {

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
  bool isUniform() const;
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
};

} // namespace rnwebos
