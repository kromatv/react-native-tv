#pragma once

#include "HostConfig.h"
#include "diagnostics/ControlLink.h"
#include "diagnostics/ErrorScreen.h"
#include "diagnostics/PerfMonitor.h"
#include "fabric/SurfacePresenter.h"
#include "input/KeyboardFocus.h"
#include "input/RemoteInput.h"
#include "modules/DeviceProfile.h"
#include "modules/LaunchUrl.h"
#include "modules/TurboModuleProvider.h"
#include "modules/dialog/AlertDialog.h"
#include "text/EvasTextMeasurer.h"
#include "threading/InstanceGate.h"
#include "threading/TaskQueueThread.h"
#include "tv/TvState.h"
#include "tv/VoiceController.h"

#include <react/runtime/ReactInstance.h>
#include <react/runtime/TimerManager.h>

#include <Evas.h>

#include <atomic>
#include <memory>
#include <string>

namespace rntizen {

class DevSupport;

/// One React Native app on a TV: its window, its Hermes runtime on a dedicated
/// JavaScript thread, its TurboModules and its Fabric surface. Every method runs
/// on the main thread.
class ReactHost final {
 public:
  /// Reads the config in `resourceDir` and starts the app; throws std::runtime_error when it cannot.
  explicit ReactHost(const std::string &resourceDir);
  ~ReactHost();

  ReactHost(const ReactHost &) = delete;
  ReactHost &operator=(const ReactHost &) = delete;

  void pause();
  void resume();

  /// Records how the app was launched: the launching app control's URI, which
  /// `Linking.getInitialURL` answers.
  void setInitialUrl(const std::string &url);

  /// Delivers a URI the app is opened with while it runs, as a `url` event.
  void openUrl(const std::string &url);

  /// Hides the window at once, then asks the app framework to terminate.
  void exit();

  /// What the shell and JavaScript share about the TV's services.
  TvState &tv();

  /// Answers Samsung's voice service; main thread only.
  VoiceController &voice();

 private:
  void start();
  void stop();
  void createWindow();
  void createInstance();
  /// Replaces the React Native instance with a new one on the same window,
  /// which fetches the bundle again: what `r` in the dev server's terminal and
  /// `DevSettings.reload()` do.
  void reload();
  void startPerfMonitor();
  void startDevSupport();
  void devSupportInstanceStarted();
  void showLogBox(bool shown);
  void loadBundle();
  void routeInput();
  void startSurface();
  void emitDeviceEvent(const std::string &eventName, folly::dynamic payload);
  void onRemoteEvent(const RemoteEvent &event);
  static void reportFatalError(const std::weak_ptr<ReactHost *> &weakHost, ErrorReport report);
  void showFatalError(const ErrorReport &report);
  void setActive(bool isActive);
  void stopReactNative();

  const HostConfig config_;
  DeviceProfile device_;
  std::shared_ptr<std::atomic<bool>> isActive_;
  std::shared_ptr<LaunchUrl> launchUrl_;
  std::shared_ptr<ReactHost *> lifetime_;
  const std::weak_ptr<ReactHost *> weakHost_;
  Evas_Object *window_{nullptr};
  std::shared_ptr<EvasTextMeasurer> textMeasurer_;
  std::shared_ptr<TaskQueueThread> jsThread_;
  std::shared_ptr<facebook::react::TimerManager> timerManager_;
  std::unique_ptr<facebook::react::ReactInstance> instance_;
  std::shared_ptr<InstanceGate> instanceGate_;
  std::shared_ptr<TurboModuleProvider> modules_;
  std::unique_ptr<SurfacePresenter> presenter_;
  std::unique_ptr<KeyboardFocus> keyboardFocus_;
  std::unique_ptr<RemoteInput> remoteInput_;
  std::unique_ptr<ErrorScreen> errorScreen_;
  std::unique_ptr<AlertDialog> alertDialog_;
  std::unique_ptr<ControlLink> controlLink_;
  std::unique_ptr<PerfMonitor> perfMonitor_;
  std::shared_ptr<TvState> tv_;
  std::unique_ptr<VoiceController> voice_;
  std::shared_ptr<DevSupport> devSupport_;
};

} // namespace rntizen
