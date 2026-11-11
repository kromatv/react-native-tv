#include "ComponentView.h"

namespace rnwebos {

// What the view and its subtree paint, in the view's own pixels: its box with
// its shadows and outline, and, unless it clips them, its children where they
// are placed and transformed. Kept until something in the subtree changes.
const SkRect &ComponentView::paintBounds() const {
  if (paintBoundsKnown_) return paintBounds_;
  const auto box = box_.overflow();
  paintBounds_ = SkRect::MakeLTRB(
      -static_cast<float>(box.left),
      -static_cast<float>(box.top),
      static_cast<float>(size_.width + box.right),
      static_cast<float>(size_.height + box.bottom));
  if (!box_.style().clips) {
    const auto shift = childrenShift();
    const SkVector moved{static_cast<float>(shift.x), static_cast<float>(shift.y)};
    for (const auto *child : children_) {
      if (!child->isOverlay()) paintBounds_.join(child->placedBounds().makeOffset(moved));
    }
  }
  paintBoundsKnown_ = true;
  return paintBounds_;
}

SkRect ComponentView::placedBounds() const {
  return transform_.mapRect(paintBounds(), pointScale())
      .makeOffset(static_cast<float>(origin_.x), static_cast<float>(origin_.y));
}

void ComponentView::paintBoundsChanged() {
  paintBoundsKnown_ = false;
  auto *view = parent_;
  while (view != nullptr && view->paintBoundsKnown_ && !view->box_.style().clips) {
    view->paintBoundsKnown_ = false;
    view = view->parent_;
  }
}

void ComponentView::childrenMoved() {
  paintBoundsChanged();
  invalidate();
}

} // namespace rnwebos
