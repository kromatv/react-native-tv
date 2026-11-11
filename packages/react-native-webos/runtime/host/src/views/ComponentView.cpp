#include "ComponentView.h"

#include <react/renderer/components/view/ViewProps.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace rnwebos {

namespace {

using facebook::react::Float;

// Children may draw past a box that does not clip them, by a transform Yoga
// knows nothing of: a view is skipped only when it is this far off screen.
constexpr float kCullSlack = 256;

int overflowPixels(Float inset, Float scale) {
  return toPixels(-std::min(Float{0}, inset), scale);
}

SkPoint point(PixelPoint pixels) {
  return {static_cast<float>(pixels.x), static_cast<float>(pixels.y)};
}

} // namespace

ComponentView::ComponentView(Scene &scene, facebook::react::Tag tag) : scene_(scene), tag_(tag) {}

ComponentView::~ComponentView() {
  if (parent_ != nullptr) parent_->unmountChild(*this);
  if (isLayer_) scene_.removeLayer(*this);
  for (auto *child : children_) {
    if (child->isLayer_) scene_.removeLayer(*child);
    child->isLayer_ = false;
    child->parent_ = nullptr;
  }
  scene_.invalidate();
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

Scene &ComponentView::scene() const {
  return scene_;
}

PixelSize ComponentView::pixelSize() const {
  return size_;
}

Float ComponentView::pointScale() const {
  return layoutMetrics_.pointScaleFactor > 0 ? layoutMetrics_.pointScaleFactor : 1;
}

void ComponentView::invalidate() {
  scene_.invalidate();
}

void ComponentView::updateProps(const facebook::react::Props::Shared &props) {
  props_ = props;
  const auto &viewProps = static_cast<const facebook::react::ViewProps &>(*props);
  opacity_ = static_cast<float>(std::clamp(viewProps.opacity, Float{0}, Float{1}));
  refreshBox();
  transform_.update(viewProps, layoutMetrics_);
  invalidate();
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
  if (child.isOverlay()) {
    scene_.addLayer(child, Scene::kModalOrder);
    child.isLayer_ = true;
  }
  child.didMount();
  didChangeChildren();
  invalidate();
}

void ComponentView::unmountChild(ComponentView &child) {
  child.willUnmount();
  std::erase(children_, &child);
  if (child.isLayer_) {
    scene_.removeLayer(child);
    child.isLayer_ = false;
  }
  child.parent_ = nullptr;
  didChangeChildren();
  invalidate();
}

void ComponentView::mountAsRoot(int order) {
  isRoot_ = true;
  isLayer_ = true;
  scene_.addLayer(*this, order);
}

void ComponentView::place() {
  const auto scale = pointScale();
  const auto &frame = layoutMetrics_.frame;
  origin_ = isOverlay() ? PixelPoint{} : PixelPoint{toPixels(frame.origin.x, scale), toPixels(frame.origin.y, scale)};
  size_ = {std::max(0, toPixels(frame.size.width, scale)), std::max(0, toPixels(frame.size.height, scale))};
  layoutContent();
  invalidate();
}

void ComponentView::refreshBox() {
  if (!props_) return;
  const auto &viewProps = static_cast<const facebook::react::ViewProps &>(*props_);
  box_.update(resolveBoxStyle(viewProps, layoutMetrics_, alwaysClips_));
}

void ComponentView::setAlwaysClips(bool alwaysClips) {
  if (alwaysClips_ == alwaysClips) return;
  alwaysClips_ = alwaysClips;
  refreshBox();
  invalidate();
}

void ComponentView::drawBackdrop(SkCanvas &, float) {
  // A plain view leaves what is behind it as it is.
}

void ComponentView::drawContent(SkCanvas &, float) {
  // A plain view draws only its box.
}

void ComponentView::drawOverChildren(SkCanvas &) {
  // A plain view draws nothing over its children but its border.
}

int ComponentView::contentLayers() const {
  return 0;
}

void ComponentView::layoutContent() {
  // A plain view has nothing to lay out but its box.
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

void ComponentView::didChangeChildren() {
  // A plain view treats every child alike.
}

PixelPoint ComponentView::childrenShift() const {
  const auto offset = childrenOffset();
  const auto scale = pointScale();
  return {toPixels(offset.x, scale), toPixels(offset.y, scale)};
}

SkRect ComponentView::paintBounds() const {
  const auto scale = pointScale();
  const auto box = box_.overflow();
  auto bounds = SkRect::MakeWH(static_cast<float>(size_.width), static_cast<float>(size_.height));
  if (box_.style().clips || children_.empty()) {
    bounds.outset(0, 0);
    return SkRect::MakeLTRB(
        bounds.left() - static_cast<float>(box.left),
        bounds.top() - static_cast<float>(box.top),
        bounds.right() + static_cast<float>(box.right),
        bounds.bottom() + static_cast<float>(box.bottom));
  }
  const auto &inset = layoutMetrics_.overflowInset;
  return SkRect::MakeLTRB(
      bounds.left() - static_cast<float>(std::max(box.left, overflowPixels(inset.left, scale))) - kCullSlack,
      bounds.top() - static_cast<float>(std::max(box.top, overflowPixels(inset.top, scale))) - kCullSlack,
      bounds.right() + static_cast<float>(std::max(box.right, overflowPixels(inset.right, scale))) + kCullSlack,
      bounds.bottom() + static_cast<float>(std::max(box.bottom, overflowPixels(inset.bottom, scale))) + kCullSlack);
}

void ComponentView::draw(SkCanvas &canvas) {
  if (layoutMetrics_.displayType == facebook::react::DisplayType::None || opacity_ <= 0) return;
  SkAutoCanvasRestore restore(&canvas, true);
  canvas.translate(static_cast<float>(origin_.x), static_cast<float>(origin_.y));
  transform_.concat(canvas, pointScale());
  const auto bounds = paintBounds();
  if (canvas.quickReject(bounds)) return;
  drawBackdrop(canvas, opacity_);
  float alpha = 1;
  if (opacity_ < 1) {
    if (children_.empty() && box_.layerCount() + contentLayers() <= 1) {
      alpha = opacity_;
    } else {
      const bool bounded = box_.style().clips || children_.empty();
      canvas.saveLayerAlphaf(bounded ? &bounds : nullptr, opacity_);
    }
  }
  box_.drawBelowContent(canvas, size_, alpha);
  {
    const bool clips = box_.style().clips;
    SkAutoCanvasRestore clipRestore(&canvas, clips);
    if (clips) box_.clipContent(canvas, size_);
    drawContent(canvas, alpha);
    drawChildren(canvas);
  }
  box_.drawAboveChildren(canvas, size_, alpha);
  drawOverChildren(canvas);
}

void ComponentView::drawChildren(SkCanvas &canvas) {
  if (children_.empty()) return;
  const auto shift = childrenShift();
  SkAutoCanvasRestore restore(&canvas, shift != PixelPoint{});
  if (shift != PixelPoint{}) canvas.translate(static_cast<float>(shift.x), static_cast<float>(shift.y));
  for (auto *child : children_) drawChild(canvas, *child);
}

void ComponentView::drawChild(SkCanvas &canvas, ComponentView &child) {
  if (!child.isOverlay()) child.draw(canvas);
}

PixelRect ComponentView::canvasRect() const {
  PixelPoint origin = origin_;
  for (const auto *view = this; view->parent_ != nullptr && !view->isOverlay(); view = view->parent_) {
    const auto shift = view->parent_->childrenShift();
    origin.x += view->parent_->origin_.x + shift.x;
    origin.y += view->parent_->origin_.y + shift.y;
  }
  return {.x = origin.x, .y = origin.y, .width = size_.width, .height = size_.height};
}

facebook::react::Point ComponentView::canvasOrigin() const {
  const auto rect = canvasRect();
  return {.x = static_cast<Float>(rect.x), .y = static_cast<Float>(rect.y)};
}

PixelRect ComponentView::screenRect() const {
  std::array<SkPoint, 4> corners{{
      {0, 0},
      {static_cast<float>(size_.width), 0},
      {static_cast<float>(size_.width), static_cast<float>(size_.height)},
      {0, static_cast<float>(size_.height)},
  }};
  for (const auto *view = this; view != nullptr; view = view->isOverlay() ? nullptr : view->parent_) {
    const auto scale = view->pointScale();
    const auto base = point(view->origin_);
    const auto shift = view->parent_ != nullptr && !view->isOverlay() ? point(view->parent_->childrenShift()) : SkPoint{};
    for (auto &corner : corners) corner = view->transform_.mapPoint(corner, scale) + base + shift;
  }
  const auto [minX, maxX] = std::minmax({corners[0].x(), corners[1].x(), corners[2].x(), corners[3].x()});
  const auto [minY, maxY] = std::minmax({corners[0].y(), corners[1].y(), corners[2].y(), corners[3].y()});
  return {
      .x = static_cast<int>(std::floor(minX)),
      .y = static_cast<int>(std::floor(minY)),
      .width = static_cast<int>(std::ceil(maxX - minX)),
      .height = static_cast<int>(std::ceil(maxY - minY)),
  };
}

ComponentView *ComponentView::hitTest(SkPoint windowPoint, SkPoint *local) {
  ComponentView *hit = nullptr;
  hitTestLocal(windowPoint, hit, local);
  return hit;
}

bool ComponentView::hitTestLocal(SkPoint outer, ComponentView *&hit, SkPoint *local) {
  if (layoutMetrics_.displayType == facebook::react::DisplayType::None) return false;
  using facebook::react::PointerEventsMode;
  const auto mode = props_ ? static_cast<const facebook::react::ViewProps &>(*props_).pointerEvents : PointerEventsMode::Auto;
  if (mode == PointerEventsMode::None) return false;
  SkPoint inner{};
  if (!transform_.invertPoint(outer - point(origin_), pointScale(), inner)) return false;
  const bool inside =
      inner.x() >= 0 && inner.y() >= 0 && inner.x() < static_cast<float>(size_.width) && inner.y() < static_cast<float>(size_.height);
  if (box_.style().clips && !inside) return false;
  if (mode != PointerEventsMode::BoxOnly) {
    const auto childPoint = inner - point(childrenShift());
    for (auto child = children_.rbegin(); child != children_.rend(); ++child) {
      if (!(*child)->isOverlay() && (*child)->hitTestLocal(childPoint, hit, local)) return true;
    }
  }
  if (!inside || mode == PointerEventsMode::BoxNone) return false;
  hit = this;
  if (local != nullptr) *local = inner;
  return true;
}

} // namespace rnwebos
