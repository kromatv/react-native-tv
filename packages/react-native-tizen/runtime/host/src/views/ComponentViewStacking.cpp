#include "ComponentView.h"

namespace rntizen {

void ComponentView::refreshClips() {
  for (auto *content : contents_) hangFromClipper(content);
  for (auto *child : children_) {
    if (!child->isOverlay()) hangChild(*child);
  }
}

void ComponentView::rehostChildren() {
  for (auto *child : children_) {
    if (child->isOverlay()) continue;
    auto *host = hostFor(*child);
    if (host == nullptr) host = root_;
    if (evas_object_smart_parent_get(child->root_) != host) evas_object_smart_member_add(child->root_, host);
  }
  refreshClips();
  restack();
}

bool ComponentView::stacksInRoot(const ComponentView &child) const {
  return !child.isOverlay() && hostFor(child) == nullptr;
}

void ComponentView::hangFromClipper(Evas_Object *object) const {
  auto *clipper = box_.clipper();
  evas_object_clip_set(object, clipper != nullptr ? clipper : evas_object_smart_clipped_clipper_get(root_));
}

void ComponentView::hangChild(const ComponentView &child) const {
  auto *clipper = clipperFor(child);
  if (clipper != nullptr) {
    evas_object_clip_set(child.root_, clipper);
  } else {
    hangFromClipper(child.root_);
  }
}

void ComponentView::stackChild(std::size_t index) {
  const auto underneath = contents_.empty() ? box_.objectsBelowContent() : contents_;
  Evas_Object *below = underneath.empty() ? nullptr : underneath.back();
  for (auto i = index; i > 0; --i) {
    if (stacksInRoot(*children_[i - 1])) {
      below = children_[i - 1]->root_;
      break;
    }
  }
  if (below != nullptr) {
    evas_object_stack_above(children_[index]->root_, below);
  } else {
    evas_object_lower(children_[index]->root_);
  }
}

void ComponentView::restack() {
  auto order = box_.objectsBelowContent();
  order.insert(order.end(), contents_.begin(), contents_.end());
  for (auto *child : children_) {
    if (stacksInRoot(*child)) order.push_back(child->root_);
  }
  const auto above = box_.objectsAboveChildren();
  order.insert(order.end(), above.begin(), above.end());
  order.insert(order.end(), overlays_.begin(), overlays_.end());
  if (order.empty()) return;
  evas_object_lower(order.front());
  for (std::size_t i = 1; i < order.size(); ++i) evas_object_stack_above(order[i], order[i - 1]);
}

} // namespace rntizen
