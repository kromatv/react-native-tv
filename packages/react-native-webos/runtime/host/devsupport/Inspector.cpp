#include "Inspector.h"

#include "InspectorTransport.h"

#include "network/Digest.h"
#include "threading/MainThread.h"

#include <cxxreact/ReactNativeVersion.h>
#include <jsinspector-modern/InspectorInterfaces.h>

#include <format>

namespace rntizen {

namespace {

using facebook::react::jsinspector_modern::getInspectorInstance;
using facebook::react::jsinspector_modern::HostCommand;
using facebook::react::jsinspector_modern::HostTarget;
using facebook::react::jsinspector_modern::HostTargetMetadata;
using facebook::react::jsinspector_modern::ILocalConnection;
using facebook::react::jsinspector_modern::InspectorPackagerConnection;
using facebook::react::jsinspector_modern::IRemoteConnection;

constexpr const char *kPageDescription = "React Native Bridgeless";

std::string hex(const std::array<std::uint8_t, 20> &digest) {
  std::string text;
  for (const auto byte : digest) text += std::format("{:02x}", byte);
  return text;
}

// Stable across launches on one TV, as Android's hash of the package and the
// device's ID is, so DevTools finds the app again after a reload.
std::string logicalDeviceId(const std::string &app, const DeviceProfile &device) {
  return hex(sha1(std::format("tizen-{}-{}-{}-fusebox", app, device.modelName, device.buildString)));
}

std::string reactNativeVersion() {
  const auto &version = facebook::react::ReactNativeVersion;
  const auto text = std::format("{}.{}.{}", version.Major, version.Minor, version.Patch);
  return version.Prerelease.empty() ? text : std::format("{}-{}", text, version.Prerelease);
}

} // namespace

Inspector::Inspector(const DevServer &server, const DeviceProfile &device, Host host)
    : app_(server.app), deviceName_(device.modelName), host_(std::move(host)) {
  target_ = HostTarget::create(*this, [](std::function<void()> &&callback) {
    if (isMainThread()) {
      callback();
      return;
    }
    runOnMainThread(std::move(callback));
  });
  pageId_ = getInspectorInstance().addPage(
      kPageDescription,
      "",
      [weakTarget = std::weak_ptr<HostTarget>(target_)](
          std::unique_ptr<IRemoteConnection> remote) -> std::unique_ptr<ILocalConnection> {
        if (auto target = weakTarget.lock()) return target->connect(std::move(remote));
        return nullptr;
      },
      {.nativePageReloads = true});
  connection_ = std::make_unique<InspectorPackagerConnection>(
      server.inspectorUrl(deviceName_, logicalDeviceId(app_, device)),
      deviceName_,
      app_,
      std::make_unique<InspectorTransport>());
  connection_->connect();
}

Inspector::~Inspector() {
  connection_->closeQuietly();
  connection_.reset();
  getInspectorInstance().removePage(pageId_);
  target_.reset();
}

HostTarget &Inspector::target() {
  return *target_;
}

void Inspector::resume() {
  target_->sendCommand(HostCommand::DebuggerResume);
}

HostTargetMetadata Inspector::getMetadata() {
  return HostTargetMetadata{
      .appDisplayName = app_,
      .appIdentifier = app_,
      .deviceName = deviceName_,
      .integrationName = "Tizen Bridgeless (ReactHost)",
      .platform = "tizen",
      .reactNativeVersion = reactNativeVersion(),
  };
}

void Inspector::onReload(const PageReloadRequest &) {
  host_.reload();
}

void Inspector::onSetPausedInDebuggerMessage(const OverlaySetPausedInDebuggerMessageRequest &request) {
  host_.showPausedMessage(request.message);
}

} // namespace rntizen
