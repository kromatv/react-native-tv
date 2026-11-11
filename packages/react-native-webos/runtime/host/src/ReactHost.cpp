#include "ReactHost.h"

#include "HostFeatureFlags.h"
#include "logging/Log.h"
#include "modules/AppStateModule.h"
#include "platform/MainLoop.h"
#include "threading/MainThread.h"
#include "tv/MediaSessions.h"

#include <chrono>
#include <format>
#include <utility>

namespace rnwebos {

namespace {

// The TV answers in a few milliseconds, while the window opens: past this the
// app starts with what it has instead of waiting for a service that is down.
constexpr auto kDeviceAnswerBudget = std::chrono::milliseconds(500);

} // namespace

ReactHost::ReactHost(const std::string &resourceDir, const LaunchParameters &launch)
    : config_(loadHostConfig(resourceDir)),
      lifetime_(std::make_shared<ReactHost *>(this)),
      weakHost_(lifetime_) {
  try {
    start(launch);
  } catch (...) {
    stop();
    throw;
  }
}

void ReactHost::start(const LaunchParameters &launch) {
  openMainThreadSync();
  const auto asked = std::chrono::steady_clock::now();
  const DeviceProfileRequest deviceRequest(LunaBus::get());
  registerApp(launch);
  overrideFeatureFlagsOnce();
  createWindow();
  readDevice(deviceRequest, asked + kDeviceAnswerBudget);
  log(LogLevel::Info,
      "Starting " + config_.appKey + " on " + device_.modelName + " (webOS " + device_.platformVersion + ", firmware " +
          device_.buildString + ", " + device_.locale + ")");
  startDevSupport();
  createInstance();
  loadBundle();
  remotePresses_ = std::make_unique<RemotePresses>([this](const RemoteEvent &event) { onRemoteEvent(event); });
  MainLoop::get().setEventHandler([this](const SDL_Event &event) { handleEvent(event); });
  startSurface();
#if RN_WEBOS_SIMULATOR
  simulatorControl_ = SimulatorControl::open({
      .pause = [this] { pause(); },
      .resume = [this] { resume(); },
      .exit = [this] { exit(); },
      .openUrl = [this](const std::string &url) { openUrl(url); },
      .relaunch =
          [this](const folly::dynamic &parameters) {
            resume();
            relaunch(parameters);
          },
  });
#endif
}

ReactHost::~ReactHost() {
  const auto started = std::chrono::steady_clock::now();
  stop();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
  log(LogLevel::Debug, std::format("React Native stopped in {} ms", elapsed.count()));
  setLogSink(nullptr);
}

void ReactHost::stop() {
  lifetime_.reset();
  registration_.reset();
  screenSaver_.reset();
#if RN_WEBOS_SIMULATOR
  simulatorControl_.reset();
#endif
  MainLoop::get().setEventHandler(nullptr);
  remotePresses_.reset();
  closeMainThreadSync();
  if (instanceGate_ != nullptr) instanceGate_->close();
  stopReactNative();
  devSupport_.reset();
  textMeasurer_.reset();
  errorScreen_.reset();
  scene_.reset();
  window_.reset();
}

void ReactHost::reload() {
  log(LogLevel::Info, "Reloading " + config_.appKey);
  remotePresses_->releaseAll();
  closeMainThreadSync();
  instanceGate_->close();
  stopReactNative();
  openMainThreadSync();
  errorScreen_ = std::make_unique<ErrorScreen>(*scene_);
  createInstance();
  loadBundle();
  startSurface();
}

void ReactHost::stopReactNative() {
  if (jsThread_ == nullptr) return;
  if (instance_ != nullptr) instance_->unregisterFromInspector();
  jsThread_->runWhileParked([this] { presenter_.reset(); });
  jsThread_->runOnQueueSync([this] {
    modules_.reset();
    // Pending timers hold JavaScript functions: the instance must free them
    // before its Hermes runtime, as on iOS, where it owns the TimerManager alone.
    timerManager_.reset();
    instance_.reset();
  });
  jsThread_->quitSynchronous();
}

void ReactHost::exit() {
  log(LogLevel::Info, "Leaving the app");
  MainLoop::get().quit();
}

void ReactHost::setActive(bool isActive) {
  if (isActive_->exchange(isActive) == isActive) return;
  emitDeviceEvent("appStateDidChange", folly::dynamic::object("app_state", AppStateModule::stateName(isActive)));
}

void ReactHost::pause() {
  remotePresses_->releaseAll();
  pauseMediaSessionsForBackground();
  setActive(false);
}

void ReactHost::resume() {
  setActive(true);
  scene_->invalidate();
}

void ReactHost::setInitialUrl(const std::string &url) {
  launch_->setInitialUrl(url);
}

void ReactHost::openUrl(const std::string &url) {
  emitDeviceEvent("url", folly::dynamic::object("url", url));
}

} // namespace rnwebos
