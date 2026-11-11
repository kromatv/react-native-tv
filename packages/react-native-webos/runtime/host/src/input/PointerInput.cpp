#include "PointerInput.h"

#include "../views/ComponentView.h"

#include <react/renderer/components/view/PointerEvent.h>
#include <react/renderer/components/view/TouchEventEmitter.h>
#include <react/renderer/components/view/ViewProps.h>

#include <algorithm>
#include <chrono>
#include <memory>

namespace rnwebos {

namespace {

using facebook::react::Float;
using facebook::react::Tag;
using facebook::react::TouchEventEmitter;
using facebook::react::ViewEvents;

constexpr int kPointerId = 1;
// A browser scrolls about this far for one notch of a wheel.
constexpr float kWheelStep = 100;

std::shared_ptr<const TouchEventEmitter> emitterOf(const ComponentView &view) {
  return std::dynamic_pointer_cast<const TouchEventEmitter>(view.eventEmitter());
}

bool listensTo(const ComponentView &view, ViewEvents::Offset bubbled, ViewEvents::Offset captured) {
  if (!view.props()) return false;
  const auto &events = static_cast<const facebook::react::ViewProps &>(*view.props()).events;
  return events[bubbled] || events[captured];
}

facebook::react::Point points(SkPoint pixels, const ComponentView &view) {
  const auto scale = view.layoutMetrics().pointScaleFactor > 0 ? view.layoutMetrics().pointScaleFactor : 1;
  return {.x = pixels.x() / scale, .y = pixels.y() / scale};
}

facebook::react::PointerEvent pointerEvent(const ComponentView &view, SkPoint window, SkPoint local, bool down) {
  facebook::react::PointerEvent event{};
  event.pointerId = kPointerId;
  event.pointerType = "mouse";
  event.clientPoint = points(window, view);
  event.screenPoint = event.clientPoint;
  event.offsetPoint = points(local, view);
  event.pressure = down ? 0.5f : 0;
  event.width = 1;
  event.height = 1;
  event.buttons = down ? 1 : 0;
  event.button = -1;
  event.isPrimary = true;
  event.timeStamp = facebook::react::HighResTimeStamp::now();
  return event;
}

facebook::react::TouchEvent touchEvent(const ComponentView &view, SkPoint window, SkPoint local, bool ended) {
  facebook::react::Touch touch{};
  touch.pagePoint = points(window, view);
  touch.screenPoint = touch.pagePoint;
  touch.offsetPoint = points(local, view);
  touch.identifier = kPointerId;
  touch.target = view.tag();
  touch.force = ended ? 0 : 1;
  touch.timeStamp = facebook::react::HighResTimeStamp::now();
  touch.timestamp = static_cast<Float>(
      std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count());
  facebook::react::TouchEvent event{};
  event.changedTouches.insert(touch);
  if (!ended) {
    event.touches.insert(touch);
    event.targetTouches.insert(touch);
  }
  return event;
}

std::vector<Tag> pathTo(const ComponentView *view) {
  std::vector<Tag> path;
  for (; view != nullptr; view = view->parent()) path.push_back(view->tag());
  std::ranges::reverse(path);
  return path;
}

} // namespace

PointerInput::PointerInput(Scene &scene, ViewFinder findView) : scene_(scene), findView_(std::move(findView)) {}

PointerInput::Target PointerInput::targetAt(SkPoint point) const {
  Target target;
  target.view = scene_.viewAt(point, &target.local);
  return target;
}

bool PointerInput::handleEvent(const SDL_Event &event) {
  switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION:
      moveTo({event.motion.x, event.motion.y});
      return true;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      if (event.button.button == SDL_BUTTON_LEFT) press({event.button.x, event.button.y});
      return true;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      if (event.button.button == SDL_BUTTON_LEFT) release({event.button.x, event.button.y}, false);
      return true;
    case SDL_EVENT_MOUSE_WHEEL:
      wheel({event.wheel.mouse_x, event.wheel.mouse_y}, event.wheel.x, event.wheel.y);
      return true;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
      pointerLeft();
      return true;
    default:
      return false;
  }
}

void PointerInput::hover(const Target &target, SkPoint point) {
  const auto path = pathTo(target.view);
  for (auto left = hovered_.rbegin(); left != hovered_.rend(); ++left) {
    if (std::ranges::find(path, *left) != path.end()) continue;
    const auto *view = findView_(*left);
    if (view == nullptr || !listensTo(*view, ViewEvents::Offset::PointerLeave, ViewEvents::Offset::PointerLeaveCapture)) {
      continue;
    }
    if (const auto emitter = emitterOf(*view)) emitter->onPointerLeave(pointerEvent(*view, point, {}, pressed_ != 0));
  }
  for (const auto tag : path) {
    if (std::ranges::find(hovered_, tag) != hovered_.end()) continue;
    const auto *view = findView_(tag);
    if (view == nullptr || !listensTo(*view, ViewEvents::Offset::PointerEnter, ViewEvents::Offset::PointerEnterCapture)) {
      continue;
    }
    if (const auto emitter = emitterOf(*view)) emitter->onPointerEnter(pointerEvent(*view, point, {}, pressed_ != 0));
  }
  hovered_ = path;
}

void PointerInput::moveTo(SkPoint point) {
  position_ = point;
  const auto target = targetAt(point);
  hover(target, point);
  if (pressed_ != 0) {
    if (const auto *view = findView_(pressed_)) {
      if (const auto emitter = emitterOf(*view)) emitter->onTouchMove(touchEvent(*view, point, target.local, false));
    }
    return;
  }
  for (const auto *view = target.view; view != nullptr; view = view->parent()) {
    if (!listensTo(*view, ViewEvents::Offset::PointerMove, ViewEvents::Offset::PointerMoveCapture)) continue;
    if (const auto emitter = emitterOf(*target.view)) {
      emitter->onPointerMove(pointerEvent(*target.view, point, target.local, false));
    }
    return;
  }
}

void PointerInput::press(SkPoint point) {
  position_ = point;
  const auto target = targetAt(point);
  hover(target, point);
  if (target.view == nullptr) return;
  pressed_ = target.view->tag();
  if (const auto emitter = emitterOf(*target.view)) {
    emitter->onTouchStart(touchEvent(*target.view, point, target.local, false));
  }
}

void PointerInput::release(SkPoint point, bool cancelled) {
  position_ = point;
  const auto tag = std::exchange(pressed_, 0);
  const auto *view = tag != 0 ? findView_(tag) : nullptr;
  if (view == nullptr) return;
  const auto emitter = emitterOf(*view);
  if (!emitter) return;
  const auto event = touchEvent(*view, point, targetAt(point).local, true);
  if (cancelled) {
    emitter->onTouchCancel(event);
  } else {
    emitter->onTouchEnd(event);
  }
}

void PointerInput::wheel(SkPoint point, float dx, float dy) {
  for (auto *view = targetAt(point).view; view != nullptr; view = view->parent()) {
    if (view->scrollByWheel({.x = -dx * kWheelStep, .y = -dy * kWheelStep})) return;
  }
}

void PointerInput::pointerLeft() {
  if (pressed_ != 0) release(position_, true);
  hover({}, position_);
}

} // namespace rnwebos
