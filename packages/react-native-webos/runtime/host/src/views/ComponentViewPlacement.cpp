#include "ComponentView.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rntizen {

namespace {

Evas_Coord overflowPixels(facebook::react::Float inset, facebook::react::Float scale) {
  return toPixels(-std::min(facebook::react::Float{0}, inset), scale);
}

} // namespace

PixelRect ComponentView::screenRect() const {
  const auto scale = pointScale();
  std::array<PixelPoint, 4> corners{{
      {.x = rect_.x, .y = rect_.y},
      {.x = rect_.x + rect_.width, .y = rect_.y},
      {.x = rect_.x + rect_.width, .y = rect_.y + rect_.height},
      {.x = rect_.x, .y = rect_.y + rect_.height},
  }};
  for (const auto *view = this; view != nullptr; view = view->isOverlay() ? nullptr : view->parent_) {
    for (auto &corner : corners) corner = view->transform_.mapPoint(corner, view->rect_, scale);
  }
  auto [minX, maxX] = std::minmax({corners[0].x, corners[1].x, corners[2].x, corners[3].x});
  auto [minY, maxY] = std::minmax({corners[0].y, corners[1].y, corners[2].y, corners[3].y});
  return {.x = minX, .y = minY, .width = maxX - minX, .height = maxY - minY};
}

PixelPoint ComponentView::childrenOrigin() const {
  const auto offset = childrenOffset();
  const auto scale = pointScale();
  return {.x = rect_.x + toPixels(offset.x, scale), .y = rect_.y + toPixels(offset.y, scale)};
}

PixelRect ComponentView::bounds() const {
  const auto scale = pointScale();
  const auto &inset = layoutMetrics_.overflowInset;
  const auto box = box_.overflow();
  return inflated(
      rect_,
      std::max(box.left, overflowPixels(inset.left, scale)),
      std::max(box.top, overflowPixels(inset.top, scale)),
      std::max(box.right, overflowPixels(inset.right, scale)),
      std::max(box.bottom, overflowPixels(inset.bottom, scale)));
}

void ComponentView::mountAsRoot() {
  setRooted(true);
  placed_ = false;
  place();
}

void ComponentView::setRooted(bool rooted) {
  if (rooted_ == rooted) return;
  rooted_ = rooted;
  if (!rooted) placed_ = false;
  for (auto *child : children_) child->setRooted(rooted);
}

void ComponentView::place() {
  if (!rooted_) return;
  const auto scale = pointScale();
  const bool inParent = parent_ != nullptr && !isOverlay();
  const auto base = inParent ? parent_->childrenOrigin() : PixelPoint{};
  const auto origin = isOverlay() ? facebook::react::Point{} : layoutMetrics_.frame.origin;
  const auto &size = layoutMetrics_.frame.size;
  const auto shift = transform_.translation(scale);
  const PixelRect rect{
      .x = base.x + toPixels(origin.x, scale) + shift.x,
      .y = base.y + toPixels(origin.y, scale) + shift.y,
      .width = std::max(0, toPixels(size.width, scale)),
      .height = std::max(0, toPixels(size.height, scale)),
  };
  const bool moved = !placed_ || rect.x != rect_.x || rect.y != rect_.y;
  rect_ = rect;
  placed_ = true;
  const auto outer = bounds();
  setGeometry(root_, outer);
  box_.layout(rect_);
  layoutMembers();
  transform_.apply(root_, rect_, outer, scale);
  if (moved) placeChildren();
}

void ComponentView::placeChildren() {
  for (auto *child : children_) child->place();
}

} // namespace rntizen
