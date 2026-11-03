#include "ContentContext.h"

#include "../mounting/LogicalScreen.h"

#include <react/renderer/components/view/TouchEventEmitter.h>
#include <react/renderer/components/view/ViewProps.h>

#include <algorithm>
#include <cctype>
#include <chrono>

namespace rntizen {

namespace {

using facebook::react::Float;
using facebook::react::Rect;

std::string normalized(const std::string &text) {
  std::string result;
  bool space = false;
  for (const unsigned char character : text) {
    if (std::isalnum(character) != 0 || character >= 0x80) {
      if (space && !result.empty()) result += ' ';
      result += static_cast<char>(std::tolower(character));
      space = false;
    } else {
      space = true;
    }
  }
  return result;
}

Rect screenFrame(const ComponentView &view) {
  const auto &metrics = view.layoutMetrics();
  const auto scale = metrics.pointScaleFactor > 0 ? metrics.pointScaleFactor : 1;
  const auto origin = view.canvasOrigin();
  return {.origin = {.x = origin.x / scale, .y = origin.y / scale}, .size = metrics.frame.size};
}

bool isOnScreen(const Rect &frame) {
  return frame.size.width > 0 && frame.size.height > 0 && frame.origin.x < kLogicalScreenSize.width &&
      frame.origin.y < kLogicalScreenSize.height && frame.origin.x + frame.size.width > 0 &&
      frame.origin.y + frame.size.height > 0;
}

bool isShown(const ComponentView &view) {
  if (!view.props() || view.layoutMetrics().displayType == facebook::react::DisplayType::None) return false;
  return static_cast<const facebook::react::ViewProps &>(*view.props()).opacity > 0;
}

void collect(ComponentView &view, std::vector<LabelledView> &found) {
  if (!isShown(view)) return;
  const auto &props = static_cast<const facebook::react::ViewProps &>(*view.props());
  if (!props.accessibilityLabel.empty()) {
    const auto frame = screenFrame(view);
    if (isOnScreen(frame)) found.push_back({.view = &view, .label = props.accessibilityLabel, .frame = frame});
    return;
  }
  for (auto *child : view.children()) collect(*child, found);
}

} // namespace

std::vector<LabelledView> labelledViews(ComponentView &root) {
  std::vector<LabelledView> found;
  for (auto *child : root.children()) collect(*child, found);
  std::stable_sort(found.begin(), found.end(), [](const LabelledView &a, const LabelledView &b) {
    const auto rowA = static_cast<int>(a.frame.origin.y / 10);
    const auto rowB = static_cast<int>(b.frame.origin.y / 10);
    return rowA != rowB ? rowA < rowB : a.frame.origin.x < b.frame.origin.x;
  });
  return found;
}

folly::dynamic contentContext(const std::vector<LabelledView> &views) {
  folly::dynamic items = folly::dynamic::array();
  for (const auto &item : views) {
    const auto centerX = item.frame.origin.x + item.frame.size.width / 2;
    const auto centerY = item.frame.origin.y + item.frame.size.height / 2;
    items.push_back(folly::dynamic::object("positionX", static_cast<int>(centerX))(
        "positionY", static_cast<int>(centerY))("title", item.label)("alias", folly::dynamic::array())(
        "bFocused", false));
  }
  return items;
}

const LabelledView *findByTitle(const std::vector<LabelledView> &views, const std::string &title) {
  const auto wanted = normalized(title);
  if (wanted.empty()) return nullptr;
  const auto match = [&views](auto &&predicate) -> const LabelledView * {
    const auto found = std::find_if(views.begin(), views.end(), predicate);
    return found == views.end() ? nullptr : &*found;
  };
  if (const auto *exact = match([&](const LabelledView &view) { return normalized(view.label) == wanted; })) {
    return exact;
  }
  if (const auto *prefix = match([&](const LabelledView &view) { return normalized(view.label).starts_with(wanted); })) {
    return prefix;
  }
  return match([&](const LabelledView &view) { return normalized(view.label).find(wanted) != std::string::npos; });
}

bool tapView(const LabelledView &item) {
  auto emitter = std::dynamic_pointer_cast<const facebook::react::TouchEventEmitter>(item.view->eventEmitter());
  if (!emitter) return false;
  const auto now = std::chrono::duration<Float>(std::chrono::steady_clock::now().time_since_epoch()).count();
  facebook::react::Touch touch{};
  touch.pagePoint = {
      .x = item.frame.origin.x + item.frame.size.width / 2, .y = item.frame.origin.y + item.frame.size.height / 2};
  touch.offsetPoint = {.x = item.frame.size.width / 2, .y = item.frame.size.height / 2};
  touch.screenPoint = touch.pagePoint;
  touch.identifier = 1;
  touch.target = item.view->tag();
  touch.force = 1;
  touch.timestamp = now;
  emitter->onTouchStart({.touches = {touch}, .changedTouches = {touch}, .targetTouches = {touch}});
  emitter->onTouchEnd({.touches = {}, .changedTouches = {touch}, .targetTouches = {}});
  return true;
}

} // namespace rntizen
