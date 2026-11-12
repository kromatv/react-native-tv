#include "ReactHost.h"

#include "HostWindow.h"
#include "tv/ScreenSize.h"
#include "views/ModalHostView.h"

#include <efl_extension.h>

#include <string>

namespace rntizen {

void ReactHost::createWindow() {
  window_ = createHostWindow(config_.appKey, device_.screenWidth, device_.screenHeight);
  const auto size = fitWindowToScreen(window_, config_.uhd, {device_.screenWidth, device_.screenHeight});
  device_.screenWidth = size.width;
  device_.screenHeight = size.height;
  keyboardFocus_ = std::make_unique<KeyboardFocus>(evas_object_evas_get(window_));
  textMeasurer_ = std::make_shared<EvasTextMeasurer>();
  overlays_.errorScreen = std::make_unique<ErrorScreen>(evas_object_evas_get(window_));
  overlays_.alertDialog = std::make_unique<AlertDialog>(evas_object_evas_get(window_));
}

void ReactHost::routeInput() {
  remoteInput_ = std::make_unique<RemoteInput>(
      window_,
      [this](const RemoteEvent &event) { onRemoteEvent(event); },
      [this](const Ecore_Event_Key &key, bool isDown) {
        if (key.key != nullptr && tv_.micButton->handleKey(key.key, isDown)) return true;
        return !overlays_.errorScreen->isShown() && !overlays_.alertDialog->isShown() &&
               keyboardFocus_->handleKey(key, isDown);
      });
  tv_.micButton->setGrab([window = window_](bool grabbed) {
    const std::string key(kMicButtonKey);
    if (grabbed) {
      eext_win_keygrab_set(window, key.c_str());
    } else {
      eext_win_keygrab_unset(window, key.c_str());
    }
  });
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
    if (!ModalHostView::requestCloseTopmost(evas_object_evas_get(window_))) emitDeviceEvent("hardwareBackPress", nullptr);
    return;
  }
  emitDeviceEvent(
      "onHWKeyEvent",
      folly::dynamic::object("eventType", std::string(event.eventType))("eventKeyAction", event.keyAction));
}

} // namespace rntizen
