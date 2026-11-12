#include "ReactHost.h"

#include "mounting/LogicalScreen.h"
#include "platform/AppPaths.h"
#include "platform/MainLoop.h"
#include "views/ModalHostView.h"

#include <chrono>

namespace rnwebos {

void ReactHost::createWindow() {
  window_ = std::make_unique<HostWindow>(
      config_.appKey, PixelSize{device_.screenWidth, device_.screenHeight}, appPaths().cache + "shaders");
  const auto size = window_->size();
  device_.screenWidth = size.width;
  device_.screenHeight = size.height;
  if (const auto rate = window_->refreshRate(); rate > 0) {
    MainLoop::get().setFramePeriod(std::chrono::nanoseconds(static_cast<std::int64_t>(1e9 / rate)));
  }
  scene_ = std::make_unique<Scene>(*window_);
  input_.keyboardFocus = std::make_unique<KeyboardFocus>(*scene_);
  textMeasurer_ = std::make_shared<SkiaTextMeasurer>(pointScaleFactor(
      {.width = static_cast<facebook::react::Float>(size.width),
       .height = static_cast<facebook::react::Float>(size.height)}));
  overlays_.errorScreen = std::make_unique<ErrorScreen>(*scene_);
  overlays_.alertDialog = std::make_unique<AlertDialog>(*scene_);
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
  if (!overlays_.errorScreen->isShown() && input_.pointerInput->handleEvent(event)) return;
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
      // The TV reports the pointer going away as a key
      // (docs/reference/webos-window-input.md, section 9).
      if (event.key.scancode == SDL_SCANCODE_WEBOS_CURSOR_HIDE) input_.pointerInput->pointerLeft();
      input_.remotePresses->keyDown(event.key);
      return;
    case SDL_EVENT_KEY_UP:
      input_.remotePresses->keyUp(event.key);
      return;
    case SDL_EVENT_TEXT_INPUT:
    case SDL_EVENT_TEXT_EDITING:
    case SDL_EVENT_SCREEN_KEYBOARD_SHOWN:
    case SDL_EVENT_SCREEN_KEYBOARD_HIDDEN:
      if (overlays_.errorScreen->isShown() || overlays_.alertDialog->isShown()) return;
      input_.keyboardFocus->handleTextEvent(event);
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
      input_.remotePresses->releaseAll();
      input_.pointerInput->pointerLeft();
      input_.keyboardFocus->releaseKeys();
      return;
    case SDL_EVENT_WINDOW_EXPOSED:
      scene_->invalidate();
      return;
    default:
      return;
  }
}

void ReactHost::onRemoteEvent(const RemoteEvent &event) {
  if (overlays_.errorScreen->isShown()) {
    if (event.isBack) exit();
    return;
  }
  if (overlays_.alertDialog->isShown()) {
    overlays_.alertDialog->handle(event);
    return;
  }
  if (event.isBack) {
    if (!ModalHostView::requestCloseTopmost(*scene_)) emitDeviceEvent("hardwareBackPress", nullptr);
    return;
  }
  emitDeviceEvent(
      "onHWKeyEvent",
      folly::dynamic::object("eventType", std::string(event.eventType))("eventKeyAction", event.keyAction));
}

} // namespace rnwebos
