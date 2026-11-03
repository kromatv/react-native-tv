#include "ScrollView.h"

#include "../mounting/ComponentViewRegistry.h"

#include <react/renderer/components/scrollview/ScrollViewEventEmitter.h>
#include <react/renderer/components/scrollview/ScrollViewProps.h>

#include <algorithm>
#include <chrono>
#include <limits>

namespace rntizen {

namespace {

using facebook::react::Float;
using facebook::react::Point;

constexpr double kFrameSeconds = 1.0 / 60;

double throttleSeconds(double milliseconds) {
  const auto seconds = milliseconds / 1000.0;
  if (seconds < 0) return std::numeric_limits<double>::infinity();
  return seconds <= kFrameSeconds ? 0 : seconds;
}

double nowSeconds() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

Float number(const folly::dynamic &value) {
  return value.isNumber() ? static_cast<Float>(value.asDouble()) : 0;
}

bool flag(const folly::dynamic &value) {
  return value.isBool() && value.asBool();
}

} // namespace

ScrollView::ScrollView(Evas *evas, facebook::react::Tag tag) : ComponentView(evas, tag) {}

void ScrollView::updateProps(const facebook::react::Props::Shared &props) {
  const auto &scrollProps = static_cast<const facebook::react::ScrollViewProps &>(*props);
  scrollEventThrottle_ = throttleSeconds(scrollProps.scrollEventThrottle);
  const bool offsetChanged = scrollProps.contentOffset != propsContentOffset_;
  propsContentOffset_ = scrollProps.contentOffset;
  ComponentView::updateProps(props);
  if (offsetChanged) scrollTo(propsContentOffset_, false);
}

void ScrollView::updateState(const facebook::react::State::Shared &state) {
  const bool first = !state_;
  state_ = std::static_pointer_cast<const facebook::react::ScrollViewShadowNode::ConcreteState>(state);
  if (!state_) return;
  if (first) {
    moveContent(state_->getData().contentOffset);
  } else {
    keepOffsetInContent();
  }
}

void ScrollView::updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) {
  ComponentView::updateLayout(layoutMetrics);
  keepOffsetInContent();
}

void ScrollView::handleCommand(const std::string &name, const folly::dynamic &args) {
  if (!args.isArray()) return;
  if (name == "scrollTo" && args.size() >= 3) {
    scrollTo({.x = number(args[0]), .y = number(args[1])}, flag(args[2]));
  } else if (name == "scrollToEnd" && !args.empty()) {
    const auto size = layoutMetrics().frame.size;
    const auto content = contentSize();
    const bool horizontal = content.width > size.width;
    scrollTo(horizontal ? Point{.x = content.width, .y = 0} : Point{.x = 0, .y = content.height}, flag(args[0]));
  }
}

Point ScrollView::childrenOffset() const {
  return {.x = -contentOffset_.x, .y = -contentOffset_.y};
}

void ScrollView::scrollTo(Point offset, bool animated) {
  const auto target = clamped(offset);
  animation_.stop();
  if (target == contentOffset_) return;
  if (!animated) {
    moveContent(target);
    finishScrolling();
    return;
  }
  animation_.start(contentOffset_, target, [this](Point step, bool finished) {
    moveContent(step);
    if (finished) {
      finishScrolling();
    } else {
      emitScroll(true);
    }
  });
}

void ScrollView::keepOffsetInContent() {
  if (animation_.isRunning() || !state_) return;
  const auto inside = clamped(contentOffset_);
  if (inside == contentOffset_) return;
  moveContent(inside);
  finishScrolling();
}

void ScrollView::moveContent(Point offset) {
  contentOffset_ = offset;
  placeChildren();
}

void ScrollView::emitScroll(bool throttled) {
  const auto now = nowSeconds();
  if (throttled && lastScrollEvent_ != 0 && now - lastScrollEvent_ <= scrollEventThrottle_) return;
  lastScrollEvent_ = now;
  if (const auto &emitter = eventEmitter()) {
    static_cast<const facebook::react::ScrollViewEventEmitter &>(*emitter).onScroll(scrollEvent());
  }
}

void ScrollView::finishScrolling() {
  emitScroll(false);
  if (const auto &emitter = eventEmitter()) {
    static_cast<const facebook::react::ScrollViewEventEmitter &>(*emitter).onMomentumScrollEnd(scrollEvent());
  }
  if (!state_ || state_->getData().contentOffset == contentOffset_) return;
  auto data = state_->getData();
  data.contentOffset = contentOffset_;
  state_->updateState(std::move(data));
}

Point ScrollView::clamped(Point offset) const {
  const auto size = layoutMetrics().frame.size;
  const auto content = contentSize();
  return {
      .x = std::clamp(offset.x, Float{0}, std::max(Float{0}, content.width - size.width)),
      .y = std::clamp(offset.y, Float{0}, std::max(Float{0}, content.height - size.height)),
  };
}

facebook::react::Size ScrollView::contentSize() const {
  if (!state_) return {};
  return state_->getData().getContentSize();
}

facebook::react::ScrollEvent ScrollView::scrollEvent() const {
  facebook::react::ScrollEvent event;
  event.contentOffset = contentOffset_;
  event.contentSize = contentSize();
  event.containerSize = layoutMetrics().frame.size;
  event.zoomScale = 1;
  event.timestamp = static_cast<Float>(nowSeconds());
  event.responderIgnoreScroll = true;
  return event;
}

RN_TIZEN_COMPONENT_VIEW("ScrollView", ScrollView);

} // namespace rntizen
