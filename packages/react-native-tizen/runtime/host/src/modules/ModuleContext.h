#pragma once

#include "DeviceProfile.h"

#include <folly/dynamic.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace rntizen {

struct AlertRequest;
struct ErrorReport;
class LaunchUrl;
class TvState;

/// What the platform's TurboModules need from the host, handed to each as it
/// is created. Every function may be called from any thread, and does nothing
/// once the host is shutting down.
struct ModuleContext {
  DeviceProfile device;
  std::string scriptURL;
  std::function<void(const std::string &eventName, folly::dynamic payload)> emitDeviceEvent;
  std::function<void()> exitApp;
  std::shared_ptr<std::atomic<bool>> isActive;
  std::function<void(const ErrorReport &report)> reportFatalError;
  std::shared_ptr<const LaunchUrl> launchUrl;
  std::function<void(AlertRequest request)> showAlert;
  std::shared_ptr<TvState> tv;
};

} // namespace rntizen
