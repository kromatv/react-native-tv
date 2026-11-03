#include "DevSupport.h"

#include "logging/Log.h"
#include "threading/MainThread.h"

#include <jsinspector-modern/InspectorFlags.h>

#include <mutex>

namespace rntizen {

namespace {

constexpr std::uint32_t kLoadTextColor = 0xffffffff;
constexpr std::uint32_t kLoadBackgroundColor = 0xff404040;

struct Current {
  std::mutex mutex;
  std::weak_ptr<DevSupport> devSupport;
};

Current &current() {
  static Current shared;
  return shared;
}

} // namespace

std::shared_ptr<DevSupport> DevSupport::start(const std::string &bundleUrl, const DeviceProfile &device, Host host) {
  auto server = devServerOf(bundleUrl);
  if (!server) {
    log(LogLevel::Warning, bundleUrl + " is not on a dev server: no Fast Refresh, reload or LogBox");
    return nullptr;
  }
  auto devSupport = std::make_shared<DevSupport>(std::move(*server), device, std::move(host));
  auto &shared = current();
  std::lock_guard lock(shared.mutex);
  shared.devSupport = devSupport;
  return devSupport;
}

void DevSupport::post(std::function<void(DevSupport &)> task) {
  std::weak_ptr<DevSupport> weak;
  {
    auto &shared = current();
    std::lock_guard lock(shared.mutex);
    weak = shared.devSupport;
  }
  if (weak.expired()) return;
  runOnMainThread([weak = std::move(weak), task = std::move(task)] {
    if (auto devSupport = weak.lock()) task(*devSupport);
  });
}

DevSupport::DevSupport(DevServer server, const DeviceProfile &device, Host host)
    : server_(std::move(server)),
      host_(std::move(host)),
      banner_(host_.evas, host_.screenWidth, host_.screenHeight) {
  packager_ = std::make_unique<PackagerConnection>(
      server_.packagerUrl(friendlyDeviceName(device)),
      PackagerConnection::Handlers{
          .reload = [this] { reload("Reload from the dev server"); },
          .devMenu =
              [] {
                log(LogLevel::Info,
                    "Tizen has no developer menu yet: press r to reload and j to open the debugger in the dev "
                    "server's terminal");
              },
      });
  if (!facebook::react::jsinspector_modern::InspectorFlags::getInstance().getFuseboxEnabled()) return;
  inspector_ = std::make_unique<Inspector>(
      server_,
      device,
      Inspector::Host{
          .reload = [this] { reload("Reload from React Native DevTools"); },
          .showPausedMessage =
              [this](const std::optional<std::string> &message) {
                if (message) {
                  showMessage(*message, std::nullopt, std::nullopt);
                } else {
                  hideMessage();
                }
              },
      });
}

facebook::react::jsinspector_modern::HostTarget *DevSupport::inspectorTarget() {
  return inspector_ ? &inspector_->target() : nullptr;
}

void DevSupport::instanceStarted(
    std::shared_ptr<InstanceGate> gate,
    const facebook::react::RuntimeExecutor &afterBundle) {
  gate_ = std::move(gate);
  settings_.instanceStarted();
  showMessage("Loading from " + server_.authority() + "…", std::nullopt, std::nullopt);
  gate_->callFunctionOnModule("HMRClient", "setup", server_.hmrClientSetup(settings_.hotLoadingEnabled()));
  afterBundle([](facebook::jsi::Runtime &) {
    post([](DevSupport &devSupport) { devSupport.hideMessage(); });
  });
}

void DevSupport::reload(const std::string &reason) {
  log(LogLevel::Info, "Reloading: " + reason);
  if (inspector_) inspector_->resume();
  host_.reload();
}

void DevSupport::setHotLoadingEnabled(bool enabled) {
  if (settings_.setHotLoadingEnabled(enabled)) reload(enabled ? "Fast Refresh turned on" : "Fast Refresh turned off");
}

void DevSupport::toggleElementInspector() {
  emitDeviceEvent("toggleElementInspector", nullptr);
}

void DevSupport::showMessage(
    const std::string &message,
    std::optional<std::uint32_t> textColor,
    std::optional<std::uint32_t> backgroundColor) {
  banner_.show(message, textColor.value_or(kLoadTextColor), backgroundColor.value_or(kLoadBackgroundColor));
}

void DevSupport::hideMessage() {
  banner_.hide();
}

void DevSupport::setLogBoxShown(bool shown) {
  if (settings_.setLogBoxShown(shown)) host_.showLogBox(shown);
}

void DevSupport::emitDeviceEvent(const std::string &name, folly::dynamic payload) {
  if (gate_ == nullptr) return;
  gate_->callFunctionOnModule("RCTDeviceEventEmitter", "emit", folly::dynamic::array(name, std::move(payload)));
}

} // namespace rntizen
