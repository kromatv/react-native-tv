#include "ReactHost.h"

#include "platform/MainLoop.h"

#include <chrono>

namespace rnwebos {

void ReactHost::createWindow() {
  window_ = std::make_unique<HostWindow>(config_.appKey, PixelSize{device_.screenWidth, device_.screenHeight});
  const auto size = window_->size();
  device_.screenWidth = size.width;
  device_.screenHeight = size.height;
  if (const auto rate = window_->refreshRate(); rate > 0) {
    MainLoop::get().setFramePeriod(std::chrono::nanoseconds(static_cast<std::int64_t>(1e9 / rate)));
  }
  scene_ = std::make_unique<Scene>(*window_);
  textMeasurer_ = std::make_shared<SkiaTextMeasurer>();
  errorScreen_ = std::make_unique<ErrorScreen>(*scene_);
}

void ReactHost::startSurface() {
  presenter_->startSurface(
      config_.appKey,
      {.width = static_cast<facebook::react::Float>(device_.screenWidth),
       .height = static_cast<facebook::react::Float>(device_.screenHeight)});
}

void ReactHost::emitDeviceEvent(const std::string &eventName, folly::dynamic payload) {
  instanceGate_->callFunctionOnModule(
      "RCTDeviceEventEmitter", "emit", folly::dynamic::array(eventName, std::move(payload)));
}

void ReactHost::handleEvent(const SDL_Event &event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
      remotePresses_->keyDown(event.key);
      return;
    case SDL_EVENT_KEY_UP:
      remotePresses_->keyUp(event.key);
      return;
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      exit();
      return;
    case SDL_EVENT_DID_ENTER_BACKGROUND:
      pause();
      return;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
      resume();
      return;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
      remotePresses_->releaseAll();
      return;
    case SDL_EVENT_WINDOW_EXPOSED:
      scene_->invalidate();
      return;
    default:
      return;
  }
}

void ReactHost::onRemoteEvent(const RemoteEvent &event) {
  if (errorScreen_->isShown()) {
    if (event.isBack) exit();
    return;
  }
  if (event.isBack) {
    emitDeviceEvent("hardwareBackPress", nullptr);
    return;
  }
  emitDeviceEvent(
      "onHWKeyEvent",
      folly::dynamic::object("eventType", std::string(event.eventType))("eventKeyAction", event.keyAction));
}

} // namespace rnwebos
