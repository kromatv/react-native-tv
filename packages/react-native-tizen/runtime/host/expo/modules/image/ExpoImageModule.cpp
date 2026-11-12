#include "ImageCache.h"

#include "../../ExpoModuleRegistry.h"
#include "../../ModuleDefinition.h"

#include "logging/Log.h"

namespace rntizen::expo {

namespace {

class ExpoImageModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoImage")
        .asyncFunction(
            "prefetch",
            [](const std::vector<std::string> &urls,
               const std::optional<std::string> &,
               const std::optional<folly::dynamic> &) {
              bool all = true;
              for (const auto &url : urls) {
                try {
                  imageFileFor(url, "");
                } catch (const std::exception &error) {
                  log(LogLevel::Debug, std::string("Image.prefetch: ") + error.what());
                  all = false;
                }
              }
              return all;
            })
        .asyncFunction("clearMemoryCache", [] { return true; })
        .asyncFunction("clearDiskCache", [] { return clearImageDiskCache(); })
        .asyncFunction("getCachePathAsync", [](const std::string &cacheKey) { return cachedImagePath(cacheKey); });
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoImage, ExpoImageModule);

} // namespace rntizen::expo
