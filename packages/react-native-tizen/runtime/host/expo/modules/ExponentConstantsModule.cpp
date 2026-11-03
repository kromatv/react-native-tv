#include "../ExpoModuleRegistry.h"
#include "../FileIo.h"
#include "../ModuleDefinition.h"
#include "../Uuid.h"
#include "DeviceName.h"

#include "logging/Log.h"

#include <folly/json.h>

namespace rntizen::expo {

namespace {

constexpr const char *kAppConfigFile = "app.config";

folly::dynamic readAppConfig(const std::string &resourceDirectory) {
  if (resourceDirectory.empty()) return nullptr;
  auto text = readFile(resourceDirectory + kAppConfigFile);
  if (!text) return nullptr;
  try {
    auto config = folly::parseJson(*text);
    return config.isObject() ? config : folly::dynamic(nullptr);
  } catch (const std::exception &error) {
    log(LogLevel::Warning, std::string("Ignoring an unreadable ") + kAppConfigFile + ": " + error.what());
    return nullptr;
  }
}

class ExponentConstantsModule final : public Module {
 public:
  ModuleDefinition definition() override {
    const auto &host = context().host();
    auto manifest = readAppConfig(context().resourceDirectory());
    return ModuleDefinition("ExponentConstants")
        .constant("manifest", [manifest] { return manifest; })
        .constant("executionEnvironment", [] { return std::string("bare"); })
        .constant("sessionId", [] {
          static const std::string session = uuidV4();
          return session;
        })
        .constant("statusBarHeight", [] { return 0; })
        .constant("deviceName", [&host] { return tvDeviceName(host.device.modelName); })
        .constant("systemFonts", [] { return std::vector<std::string>(); })
        .constant("systemVersion", [&host] { return host.device.platformVersion; })
        .constant("debugMode", [] { return false; })
        .constant("isHeadless", [] { return false; })
        .asyncFunction("getWebViewUserAgentAsync", [] { return std::optional<std::string>(); });
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExponentConstants, ExponentConstantsModule);

} // namespace rntizen::expo
