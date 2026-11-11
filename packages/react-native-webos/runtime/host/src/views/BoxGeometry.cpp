#include "BoxGeometry.h"

#include <algorithm>

namespace rnwebos {

bool Edges::isZero() const {
  return left == 0 && top == 0 && right == 0 && bottom == 0;
}

bool Edges::isUniform() const {
  return left == top && top == right && right == bottom;
}

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

} // namespace rnwebos
