#include "ReactHost.h"

#include "HostFeatureFlags.h"
#include "diagnostics/CrashReporter.h"
#include "diagnostics/DevelopmentLinks.h"
#include "logging/Log.h"
#include "modules/AppStateModule.h"
#include "threading/MainThread.h"
#include "tv/MediaSessions.h"

#include <app.h>

#include <chrono>
#include <utility>

namespace rntizen {

ReactHost::ReactHost(const std::string &resourceDir)
    : config_(loadHostConfig(resourceDir)),
      device_(readDeviceProfile()),
      isActive_(std::make_shared<std::atomic<bool>>(true)),
      launchUrl_(std::make_shared<LaunchUrl>()),
      lifetime_(std::make_shared<ReactHost *>(this)),
      weakHost_(lifetime_) {
  try {
    start();
  } catch (...) {
    stop();
    throw;
  }
}

void ReactHost::start() {
  openMainThreadSync();
  controlLink_ = openDevelopmentLinks(config_);
  installCrashReporter();
  log(LogLevel::Info,
      "Starting " + config_.appKey + " on " + device_.modelName + " (Tizen " + device_.platformVersion + ")");
  overrideFeatureFlagsOnce();
  tv_ = std::make_shared<TvState>(config_.voice, config_.voiceSearchUrl);
  createWindow();
  startDevSupport();
  createInstance();
  startPerfMonitor();
  loadBundle();
  routeInput();
  startSurface();
  voice_ = std::make_unique<VoiceController>(VoiceController::Dependencies{
      .state = tv_,
      .rootView = [this] { return presenter_ ? presenter_->rootView() : nullptr; },
      .emitDeviceEvent = [this](const std::string &event, folly::dynamic payload) {
        emitDeviceEvent(event, std::move(payload));
      },
      .openUrl = [this](const std::string &url) { openUrl(url); },
  });
}

ReactHost::~ReactHost() {
  const auto started = std::chrono::steady_clock::now();
  stop();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
  log(LogLevel::Debug, "React Native stopped in " + std::to_string(elapsed.count()) + " ms");
  setLogSink(nullptr);
}

void ReactHost::stop() {
  lifetime_.reset();
  perfMonitor_.reset();
  remoteInput_.reset();
  closeMainThreadSync();
  if (instanceGate_ != nullptr) instanceGate_->close();
  stopReactNative();
  devSupport_.reset();
  keyboardFocus_.reset();
  textMeasurer_.reset();
  alertDialog_.reset();
  errorScreen_.reset();
  if (window_ != nullptr) evas_object_del(std::exchange(window_, nullptr));
}

void ReactHost::startPerfMonitor() {
  if (controlLink_ == nullptr) return;
  perfMonitor_ = std::make_unique<PerfMonitor>(PerfMonitor::Sources{
      .evas = evas_object_evas_get(window_),
      .controlLink = controlLink_.get(),
      .jsThread = jsThread_,
      .runtimeExecutor = instance_->getBufferedRuntimeExecutor(),
  });
}

void ReactHost::reload() {
  log(LogLevel::Info, "Reloading " + config_.appKey);
  perfMonitor_.reset();
  remoteInput_->releaseAll();
  closeMainThreadSync();
  instanceGate_->close();
  stopReactNative();
  openMainThreadSync();
  auto *evas = evas_object_evas_get(window_);
  errorScreen_ = std::make_unique<ErrorScreen>(evas);
  alertDialog_ = std::make_unique<AlertDialog>(evas);
  createInstance();
  startPerfMonitor();
  loadBundle();
  startSurface();
}

void ReactHost::stopReactNative() {
  if (jsThread_ == nullptr) return;
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
  evas_object_hide(window_);
  log(LogLevel::Info, "Leaving the app");
  ui_app_exit();
}

void ReactHost::setActive(bool isActive) {
  if (isActive_->exchange(isActive) == isActive) return;
  emitDeviceEvent("appStateDidChange", folly::dynamic::object("app_state", AppStateModule::stateName(isActive)));
}

void ReactHost::pause() {
  remoteInput_->releaseAll();
  pauseMediaSessionsForBackground(config_.backgroundMedia);
  setActive(false);
}

void ReactHost::resume() {
  setActive(true);
}

void ReactHost::setInitialUrl(const std::string &url) {
  launchUrl_->set(url);
}

void ReactHost::openUrl(const std::string &url) {
  emitDeviceEvent("url", folly::dynamic::object("url", url));
}

TvState &ReactHost::tv() {
  return *tv_;
}

VoiceController &ReactHost::voice() {
  return *voice_;
}

} // namespace rntizen
