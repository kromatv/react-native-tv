#include "BorderSides.h"

namespace rntizen {

bool RingPart::isWhole() const {
  return sides == SideSet::all();
}

std::optional<Side> SideJoins::sideAt(float x, float y) const {
  const auto fromRight = static_cast<float>(width) - x;
  const auto fromBottom = static_cast<float>(height) - y;
  const bool left = x < static_cast<float>(reach.left);
  const bool right = !left && fromRight <= static_cast<float>(reach.right);
  const bool top = y < static_cast<float>(reach.top);
  const bool bottom = !top && fromBottom <= static_cast<float>(reach.bottom);
  using enum Side;
  const auto vertical = left ? Left : Right;
  const auto horizontal = top ? Top : Bottom;
  if (!left && !right) return top || bottom ? std::optional{horizontal} : std::nullopt;
  if (!top && !bottom) return vertical;
  const auto across = left ? x : fromRight;
  const auto down = top ? y : fromBottom;
  const auto acrossWidth = static_cast<float>(left ? widths.left : widths.right);
  const auto downWidth = static_cast<float>(top ? widths.top : widths.bottom);
  return down * acrossWidth < across * downWidth ? horizontal : vertical;
}

} // namespace rntizen
