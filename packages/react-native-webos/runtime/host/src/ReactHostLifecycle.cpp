#include "ReactHost.h"

#include "intl/IcuData.h"
#include "logging/Log.h"
#include "platform/BackAtRoot.h"

#include <SDL3/SDL_video.h>

#include <cstdlib>

namespace rnwebos {

namespace {

// What `rn-webos` writes into appinfo.json as nativeLifeCycleInterfaceVersion,
// for when the launch argument does not name the interface.
constexpr int kLifecycleInterface = 2;

} // namespace

void ReactHost::registerApp(const LaunchParameters &launch) {
  launch_->setParameters(launch.parameters);
  if (const auto url = deepLinkOf(launch.parameters)) launch_->setInitialUrl(*url);
  auto &bus = LunaBus::get();
  const char *appId = std::getenv("APPID");
  if (!bus.isAvailable() || appId == nullptr || *appId == '\0') return;
  registration_ = AppRegistration::start(
      bus,
      appId,
      launch.interfaceVersion.value_or(kLifecycleInterface),
      {
          .relaunch = [this](const folly::dynamic &parameters, const std::string &) { relaunch(parameters); },
          .pause = [this] { pause(); },
          .close = [this] { exit(); },
      });
  if (registration_ == nullptr) log(LogLevel::Warning, "Cannot register with the app manager: the TV may close the app");
  screenSaver_ = ScreenSaver::watch(bus, appId);
}

void ReactHost::readDevice(const DeviceProfileRequest &request, std::chrono::steady_clock::time_point deadline) {
  request.finish(device_, deadline);
  if (device_.panelWidth == 0) {
    device_.panelWidth = device_.screenWidth;
    device_.panelHeight = device_.screenHeight;
  }
  if (device_.locale.empty()) {
    device_.locale = defaultLanguageTag();
  } else {
    useDeviceLocale(device_.locale);
  }
}

void ReactHost::relaunch(const folly::dynamic &parameters) {
  log(LogLevel::Info, "Launched again while running");
  launch_->setParameters(parameters);
  // The TV shows a hidden app again only once its surface asks for it
  // (docs/reference/webos-window-input.md, section 3).
  SDL_RaiseWindow(window_->sdlWindow());
  emitDeviceEvent("webOSRelaunch", folly::dynamic::object("parameters", parameters));
  if (const auto url = deepLinkOf(parameters)) openUrl(*url);
}

void ReactHost::leave() {
  leaveOnBackAtRoot(LunaBus::get(), kBackAtRoot, [weakHost = weakHost_] {
    if (auto host = weakHost.lock()) (*host)->exit();
  });
}

} // namespace rnwebos
