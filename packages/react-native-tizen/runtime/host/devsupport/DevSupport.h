#pragma once

#include "DevServer.h"
#include "DevSettingsState.h"
#include "Inspector.h"
#include "LoadingBanner.h"
#include "PackagerConnection.h"

#include "modules/DeviceProfile.h"
#include "threading/InstanceGate.h"

#include <ReactCommon/RuntimeExecutor.h>
#include <folly/dynamic.h>

#include <Evas.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace rntizen {

/// What a development runtime adds to an app whose bundle comes from a dev
/// server, as Android's DevSupportManager does: the packager socket, the HMR
/// client's setup (Fast Refresh, and logs in the dev server's terminal),
/// reload, LogBox's surface, the loading banner and the inspector React Native
/// DevTools attaches to. The host keeps it across reloads. Create, use and
/// destroy it on the main thread.
class DevSupport final {
 public:
  struct Host {
    Evas *evas;
    int screenWidth;
    int screenHeight;
    /// Tears the React Native instance down and starts a new one.
    std::function<void()> reload;
    /// Starts or stops the surface `AppRegistry` registers as `LogBox`.
    std::function<void(bool shown)> showLogBox;
  };

  /// Starts the tooling for the bundle at `bundleUrl`; null, logged, when it is
  /// not on an `http` or `https` server.
  static std::shared_ptr<DevSupport> start(const std::string &bundleUrl, const DeviceProfile &device, Host host);

  /// Runs `task` on the main thread with the running app's tooling, if it has
  /// any. Any thread: how its TurboModules reach it.
  static void post(std::function<void(DevSupport &)> task);

  DevSupport(DevServer server, const DeviceProfile &device, Host host);

  DevSupport(const DevSupport &) = delete;
  DevSupport &operator=(const DevSupport &) = delete;

  /// A new instance is about to fetch and run the bundle. `afterBundle` runs
  /// a task once the bundle has run, as the instance's buffered executor does.
  void instanceStarted(std::shared_ptr<InstanceGate> gate, const facebook::react::RuntimeExecutor &afterBundle);

  /// What every instance registers with, so DevTools can debug it; null when
  /// the runtime has no debugger.
  facebook::react::jsinspector_modern::HostTarget *inspectorTarget();

  void reload(const std::string &reason);
  void setHotLoadingEnabled(bool enabled);
  void toggleElementInspector();
  void showMessage(
      const std::string &message,
      std::optional<std::uint32_t> textColor,
      std::optional<std::uint32_t> backgroundColor);
  void hideMessage();
  void setLogBoxShown(bool shown);

 private:
  void emitDeviceEvent(const std::string &name, folly::dynamic payload);

  const DevServer server_;
  const Host host_;
  LoadingBanner banner_;
  std::unique_ptr<PackagerConnection> packager_;
  std::unique_ptr<Inspector> inspector_;
  std::shared_ptr<InstanceGate> gate_;
  DevSettingsState settings_;
};

} // namespace rntizen
