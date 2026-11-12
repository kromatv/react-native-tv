#pragma once

#include "DevServer.h"

#include "modules/DeviceProfile.h"

#include <jsinspector-modern/HostTarget.h>
#include <jsinspector-modern/InspectorPackagerConnection.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace rntizen {

/// React Native DevTools' way into the app, as Android's ReactHostInspectorTarget
/// and DevServerHelper.openInspectorConnection give it: a HostTarget the
/// instances register with, listed as an inspector page, and the socket to
/// the dev server's `/inspector/device`. Create, use and destroy it on the
/// main thread, which is the inspector queue; an instance must unregister
/// before it goes.
class Inspector final : public facebook::react::jsinspector_modern::HostTargetDelegate {
 public:
  struct Host {
    std::function<void()> reload;
    /// Shows DevTools' "Paused in debugger" message, or hides it on nullopt.
    std::function<void(const std::optional<std::string> &message)> showPausedMessage;
  };

  Inspector(const DevServer &server, const DeviceProfile &device, Host host);
  ~Inspector() override;

  Inspector(const Inspector &) = delete;
  Inspector &operator=(const Inspector &) = delete;

  facebook::react::jsinspector_modern::HostTarget &target();

  /// Lets a runtime paused at a breakpoint run on, so it can be torn down.
  void resume();

  facebook::react::jsinspector_modern::HostTargetMetadata getMetadata() override;
  void onReload(const PageReloadRequest &request) override;
  void onSetPausedInDebuggerMessage(const OverlaySetPausedInDebuggerMessageRequest &request) override;

 private:
  const std::string app_;
  const std::string deviceName_;
  const Host host_;
  std::shared_ptr<facebook::react::jsinspector_modern::HostTarget> target_;
  int pageId_;
  std::unique_ptr<facebook::react::jsinspector_modern::InspectorPackagerConnection> connection_;
};

} // namespace rntizen
