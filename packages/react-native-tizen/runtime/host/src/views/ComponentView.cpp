#include "ComponentView.h"

#include "RenderSync.h"
#include "ViewSmartObject.h"

#include <react/renderer/components/view/ViewProps.h>

#include <algorithm>
#include <cmath>

namespace rntizen {

namespace {

constexpr int kOverlayLayer = 1000;

int opacityByte(facebook::react::Float opacity) {
  return static_cast<int>(std::lround(std::clamp(opacity, facebook::react::Float{0}, facebook::react::Float{1}) * 255));
}

} // namespace

ComponentView::SmartObject::~SmartObject() {
  if (evas_object_map_enable_get(object)) waitForRenderThread(object);
  evas_object_del(object);
}

ComponentView::ComponentView(Evas *evas, facebook::react::Tag tag)
    : evas_(evas), tag_(tag), smart_{addViewSmartObject(evas)}, root_(smart_.object), box_(evas, root_) {
  box_.update({});
  evas_object_show(root_);
}

ComponentView::~ComponentView() {
  if (parent_ != nullptr) parent_->unmountChild(*this);
  for (auto *child : children_) {
    detach(*child);
    child->parent_ = nullptr;
  }
}

facebook::react::Tag ComponentView::tag() const {
  return tag_;
}

ComponentView *ComponentView::parent() const {
  return parent_;
}

std::span<ComponentView *const> ComponentView::children() const {
  return children_;
}

const facebook::react::LayoutMetrics &ComponentView::layoutMetrics() const {
  return layoutMetrics_;
}

const facebook::react::Props::Shared &ComponentView::props() const {
  return props_;
}

const facebook::react::EventEmitter::Shared &ComponentView::eventEmitter() const {
  return eventEmitter_;
}

Evas *ComponentView::evas() const {
  return evas_;
}

Evas_Object *ComponentView::root() const {
  return root_;
}

Evas_Object *ComponentView::rootOf(const ComponentView &view) {
  return view.root_;
}

const PixelRect &ComponentView::canvasRect() const {
  return rect_;
}

facebook::react::Point ComponentView::canvasOrigin() const {
  return {.x = static_cast<facebook::react::Float>(rect_.x), .y = static_cast<facebook::react::Float>(rect_.y)};
}

facebook::react::Float ComponentView::pointScale() const {
  return layoutMetrics_.pointScaleFactor > 0 ? layoutMetrics_.pointScaleFactor : 1;
}

void ComponentView::updateProps(const facebook::react::Props::Shared &props) {
  props_ = props;
  const auto &viewProps = static_cast<const facebook::react::ViewProps &>(*props);
  const auto opacity = opacityByte(viewProps.opacity);
  evas_object_color_set(root_, opacity, opacity, opacity, opacity);
  refreshBox();
  transform_.update(viewProps, layoutMetrics_);
  if (placed_) place();
}

void ComponentView::updateState(const facebook::react::State::Shared &) {
  // A plain view has no native state.
}

void ComponentView::updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) {
  const bool reshaped = layoutMetrics.frame.size != layoutMetrics_.frame.size ||
      layoutMetrics.borderWidth != layoutMetrics_.borderWidth ||
      layoutMetrics.pointScaleFactor != layoutMetrics_.pointScaleFactor ||
      layoutMetrics.layoutDirection != layoutMetrics_.layoutDirection;
  layoutMetrics_ = layoutMetrics;
  if (reshaped) {
    refreshBox();
    if (props_) transform_.update(static_cast<const facebook::react::ViewProps &>(*props_), layoutMetrics_);
  }
  applyVisibility();
  place();
}

void ComponentView::updateEventEmitter(const facebook::react::EventEmitter::Shared &eventEmitter) {
  eventEmitter_ = eventEmitter;
}

void ComponentView::handleCommand(const std::string &, const folly::dynamic &) {
  // A plain view implements no command, so it drops them all.
}

void ComponentView::mountChild(ComponentView &child, std::size_t index) {
  index = std::min(index, children_.size());
  child.parent_ = this;
  children_.insert(children_.begin() + static_cast<std::ptrdiff_t>(index), &child);
  attach(child);
  if (rooted_) {
    child.setRooted(true);
    child.placed_ = false;
    child.place();
  }
  if (!child.isOverlay()) stackChild(index);
  child.didMount();
  didChangeChildren();
}

void ComponentView::unmountChild(ComponentView &child) {
  child.willUnmount();
  std::erase(children_, &child);
  detach(child);
  child.parent_ = nullptr;
  child.setRooted(false);
  didChangeChildren();
}

void ComponentView::attach(ComponentView &child) {
  child.detached_ = false;
  if (child.isOverlay()) {
    evas_object_layer_set(child.root_, kOverlayLayer);
    evas_object_raise(child.root_);
  } else {
    evas_object_smart_member_add(child.root_, root_);
    hangChild(child);
  }
  child.applyVisibility();
}

void ComponentView::detach(ComponentView &child) {
  if (!child.isOverlay()) evas_object_smart_member_del(child.root_);
  child.detached_ = true;
  child.applyVisibility();
}

void ComponentView::applyVisibility() {
  const bool hidden = detached_ || layoutMetrics_.displayType == facebook::react::DisplayType::None;
  if (hidden) {
    evas_object_hide(root_);
  } else {
    evas_object_show(root_);
  }
}

void ComponentView::addMember(Evas_Object *member, bool belowChildren) {
  evas_object_smart_member_add(member, root_);
  evas_object_show(member);
  if (belowChildren) {
    contents_.push_back(member);
    hangFromClipper(member);
  } else {
    overlays_.push_back(member);
  }
  restack();
}

void ComponentView::layoutMembers() {
  // A plain view draws only its box, which `place` lays out.
}

facebook::react::Point ComponentView::childrenOffset() const {
  return {};
}

bool ComponentView::isOverlay() const {
  return false;
}

void ComponentView::didMount() {
  // A plain view has nothing to start once mounted.
}

void ComponentView::willUnmount() {
  // A plain view has nothing to stop before it leaves.
}

Evas_Object *ComponentView::clipperFor(const ComponentView &) const {
  return nullptr;
}

void ComponentView::didChangeChildren() {
  // A plain view treats every child alike.
}

void ComponentView::setAlwaysClips(bool alwaysClips) {
  if (alwaysClips_ == alwaysClips) return;
  alwaysClips_ = alwaysClips;
  refreshBox();
  if (placed_) place();
}

void ComponentView::refreshBox() {
  if (!props_) return;
  const auto &viewProps = static_cast<const facebook::react::ViewProps &>(*props_);
  if (!box_.update(resolveBoxStyle(viewProps, layoutMetrics_, alwaysClips_))) return;
  refreshClips();
  restack();
}

void ComponentView::refreshClips() {
  for (auto *content : contents_) hangFromClipper(content);
  for (auto *child : children_) {
    if (!child->isOverlay()) hangChild(*child);
  }
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
    if (!children_[i - 1]->isOverlay()) {
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
    if (!child->isOverlay()) order.push_back(child->root_);
  }
  const auto above = box_.objectsAboveChildren();
  order.insert(order.end(), above.begin(), above.end());
  order.insert(order.end(), overlays_.begin(), overlays_.end());
  if (order.empty()) return;
  evas_object_lower(order.front());
  for (std::size_t i = 1; i < order.size(); ++i) evas_object_stack_above(order[i], order[i - 1]);
}

} // namespace rntizen
