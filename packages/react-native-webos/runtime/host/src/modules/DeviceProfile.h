#pragma once

#include "../platform/LunaBus.h"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

namespace rnwebos {

/// What the TV says about itself, asked once at start.
struct DeviceProfile {
  std::string modelName;
  /// The webOS TV platform version, such as `10.2.0`: the TV's `sdkVersion`.
  std::string platformVersion;
  /// The firmware version, such as `33.22.52`.
  std::string buildString;
  /// The surface the app draws into, in pixels.
  int screenWidth;
  int screenHeight;
  /// The panel, in pixels: larger than the surface on an Ultra HD TV.
  int panelWidth{0};
  int panelHeight{0};
  /// The language of the TV's menus as BCP 47, such as `en-GB`; empty when the TV did not say.
  std::string locale;
};

/// Looks an environment variable up: its value, or null when it is not set.
using EnvironmentLookup = std::function<const char *(const char *name)>;

/// The profile before the TV answers: what `environment` says, as the
/// simulator sets it (`RN_WEBOS_MODEL_NAME`, `RN_WEBOS_PLATFORM_VERSION`,
/// `RN_WEBOS_BUILD`, `RN_WEBOS_LOCALE`), else placeholders, on a 1920x1080
/// screen until the window says otherwise.
DeviceProfile readDeviceProfile(
    const EnvironmentLookup &environment = [](const char *name) { return std::getenv(name); });

/// Puts the reply of the TV's `getSystemInfo` into `profile`. False when it is an error.
bool applySystemInfo(DeviceProfile &profile, const folly::dynamic &reply);

/// Puts the menu language of a `getSystemSettings` reply for `localeInfo` into `profile`. False when it has none.
bool applyLocaleInfo(DeviceProfile &profile, const folly::dynamic &reply);

/// The questions that fill a DeviceProfile, sent when this is made so the TV
/// answers while the app opens its window.
class DeviceProfileRequest final {
 public:
  explicit DeviceProfileRequest(LunaBus &bus);

  /// Puts what the TV answered by `deadline` into `profile`, and logs what it did not.
  void finish(DeviceProfile &profile, std::chrono::steady_clock::time_point deadline) const;

 private:
  const bool available_;
  const std::shared_ptr<LunaBus::PendingReply> systemInfo_;
  const std::shared_ptr<LunaBus::PendingReply> localeInfo_;
};

} // namespace rnwebos
