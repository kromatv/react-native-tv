#include "ComponentView.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rntizen {

namespace {

// How far past a clipping view's box its content is still placed and drawn, in
// points: room for what a view draws past the bounds Fabric knows, as a focus
// glow or a scale.
constexpr facebook::react::Float kShownRoom = 160;

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

Edges ComponentView::ownOverflow() const {
  const auto scale = pointScale();
  const auto &inset = layoutMetrics_.overflowInset;
  const auto box = box_.overflow();
  return {
      .left = std::max(box.left, overflowPixels(inset.left, scale)),
      .top = std::max(box.top, overflowPixels(inset.top, scale)),
      .right = std::max(box.right, overflowPixels(inset.right, scale)),
      .bottom = std::max(box.bottom, overflowPixels(inset.bottom, scale)),
  };
}

Edges ComponentView::paintedOverflow() const {
  auto edges = ownOverflow();
  if (box_.clipper() != nullptr) return edges;
  const auto scale = pointScale();
  const auto offset = childrenOffset();
  const auto width = toPixels(layoutMetrics_.frame.size.width, scale);
  const auto height = toPixels(layoutMetrics_.frame.size.height, scale);
  for (const auto *child : children_) {
    if (child->isOverlay() || child->layoutMetrics_.displayType == facebook::react::DisplayType::None) continue;
    const auto reach = child->paintedOverflow();
    const auto shift = child->transform_.translation(scale);
    const auto &frame = child->layoutMetrics_.frame;
    const auto left = toPixels(frame.origin.x + offset.x, scale) + shift.x;
    const auto top = toPixels(frame.origin.y + offset.y, scale) + shift.y;
    const auto right = left + toPixels(frame.size.width, scale);
    const auto bottom = top + toPixels(frame.size.height, scale);
    edges.left = std::max(edges.left, reach.left - left);
    edges.top = std::max(edges.top, reach.top - top);
    edges.right = std::max(edges.right, right + reach.right - width);
    edges.bottom = std::max(edges.bottom, bottom + reach.bottom - height);
  }
  return edges;
}

// Evas renders a mapped smart object into a surface the size of its geometry
// (efl-rendering.md section 3): a mapped view's reaches as far as anything
// under it draws, a child's shadow or outline included, which the layout
// overflow Fabric reports leaves out.
PixelRect ComponentView::bounds() const {
  const auto edges = transform_.isMapped() ? paintedOverflow() : ownOverflow();
  return inflated(rect_, edges.left, edges.top, edges.right, edges.bottom);
}

void ComponentView::growMappedAncestors() const {
  for (auto *ancestor = parent_; ancestor != nullptr; ancestor = ancestor->parent_) {
    if (ancestor->placed_ && ancestor->transform_.isMapped()) ancestor->place();
  }
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
  const auto region = inParent ? parent_->childrenRegion_ : std::nullopt;
  const bool moved = !placed_ || rect.x != rect_.x || rect.y != rect_.y;
  rect_ = rect;
  placed_ = true;
  const auto outer = bounds();
  const bool culled = region.has_value() && !transform_.isMapped() && !overlap(outer, *region);
  const bool returned = culled_ && !culled;
  if (culled != culled_) {
    culled_ = culled;
    applyVisibility();
  }
  if (culled_) return;
  setGeometry(root_, outer);
  box_.layout(rect_);
  layoutMembers();
  transform_.apply(root_, rect_, outer, scale);
  auto forChildren = visibleRegionForChildren(region);
  const bool narrowed = forChildren != childrenRegion_;
  childrenRegion_ = std::move(forChildren);
  if (moved || narrowed || returned) placeChildren();
}

std::optional<PixelRect> ComponentView::visibleRegionForChildren(const std::optional<PixelRect> &outside) const {
  if (box_.clipper() == nullptr) return outside;
  const auto room = toPixels(kShownRoom, pointScale());
  const auto shown = inflated(rect_, room, room, room, room);
  return outside.has_value() ? intersection(shown, *outside) : shown;
}

void ComponentView::placeChildren() {
  for (auto *child : children_) child->place();
}

} // namespace rntizen
