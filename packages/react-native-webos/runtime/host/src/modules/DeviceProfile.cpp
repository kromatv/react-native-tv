#include "DeviceProfile.h"

#include "../logging/Log.h"

namespace rnwebos {

namespace {

constexpr const char *kGetSystemInfo = "luna://com.webos.service.tv.systemproperty/getSystemInfo";
constexpr const char *kGetSystemSettings = "luna://com.webos.settingsservice/getSystemSettings";
constexpr std::size_t kMaxFieldBytes = 256;

std::string valueOr(const EnvironmentLookup &environment, const char *name, const char *fallback) {
  const char *value = environment(name);
  return value != nullptr && *value != '\0' ? value : fallback;
}

bool succeeded(const folly::dynamic &reply) {
  if (!reply.isObject()) return false;
  const auto *returnValue = reply.get_ptr("returnValue");
  return returnValue != nullptr && returnValue->isBool() && returnValue->getBool();
}

void assignString(std::string &field, const folly::dynamic &object, const char *key) {
  const auto *value = object.get_ptr(key);
  if (value == nullptr || !value->isString()) return;
  const auto &text = value->getString();
  if (!text.empty() && text.size() <= kMaxFieldBytes) field = text;
}

} // namespace

DeviceProfile readDeviceProfile(const EnvironmentLookup &environment) {
  return DeviceProfile{
      .modelName = valueOr(environment, "RN_WEBOS_MODEL_NAME", "LG TV"),
      .platformVersion = valueOr(environment, "RN_WEBOS_PLATFORM_VERSION", "unknown"),
      .buildString = valueOr(environment, "RN_WEBOS_BUILD", "unknown"),
      .screenWidth = 1920,
      .screenHeight = 1080,
      .locale = valueOr(environment, "RN_WEBOS_LOCALE", ""),
  };
}

bool applySystemInfo(DeviceProfile &profile, const folly::dynamic &reply) {
  if (!succeeded(reply)) return false;
  assignString(profile.modelName, reply, "modelName");
  assignString(profile.platformVersion, reply, "sdkVersion");
  assignString(profile.buildString, reply, "firmwareVersion");
  // The TV gives no panel size here, only whether the panel is Ultra HD
  // (docs/reference/webos-tv-platform.md, section 6).
  std::string uhd;
  assignString(uhd, reply, "UHD");
  if (uhd == "true") {
    profile.panelWidth = 3840;
    profile.panelHeight = 2160;
  } else if (uhd == "false") {
    profile.panelWidth = 1920;
    profile.panelHeight = 1080;
  }
  return true;
}

bool applyLocaleInfo(DeviceProfile &profile, const folly::dynamic &reply) {
  if (!succeeded(reply)) return false;
  const auto *settings = reply.get_ptr("settings");
  const auto *localeInfo = settings != nullptr && settings->isObject() ? settings->get_ptr("localeInfo") : nullptr;
  const auto *locales = localeInfo != nullptr && localeInfo->isObject() ? localeInfo->get_ptr("locales") : nullptr;
  if (locales == nullptr || !locales->isObject()) return false;
  std::string menuLanguage;
  assignString(menuLanguage, *locales, "UI");
  if (menuLanguage.empty()) return false;
  profile.locale = menuLanguage;
  return true;
}

DeviceProfileRequest::DeviceProfileRequest(LunaBus &bus)
    : available_(bus.isAvailable()),
      systemInfo_(bus.ask(
          kGetSystemInfo,
          folly::dynamic::object("keys", folly::dynamic::array("modelName", "firmwareVersion", "sdkVersion", "UHD")))),
      localeInfo_(bus.ask(kGetSystemSettings, folly::dynamic::object("keys", folly::dynamic::array("localeInfo")))) {}

void DeviceProfileRequest::finish(DeviceProfile &profile, std::chrono::steady_clock::time_point deadline) const {
  if (!available_) return;
  const auto systemInfo = systemInfo_ != nullptr ? systemInfo_->waitUntil(deadline) : std::nullopt;
  if (!systemInfo || !applySystemInfo(profile, *systemInfo)) {
    log(LogLevel::Warning, "The TV did not say its model and version: they read as unknown");
  }
  const auto localeInfo = localeInfo_ != nullptr ? localeInfo_->waitUntil(deadline) : std::nullopt;
  if (!localeInfo || !applyLocaleInfo(profile, *localeInfo)) {
    log(LogLevel::Warning, "The TV did not say its language: the app keeps the one of its environment");
  }
}

} // namespace rnwebos
