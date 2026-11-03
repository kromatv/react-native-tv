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
            [](std::vector<std::string> urls, std::optional<std::string>, std::optional<folly::dynamic>) {
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
        .asyncFunction("getCachePathAsync", [](std::string cacheKey) { return cachedImagePath(cacheKey); });
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoImage, ExpoImageModule);

} // namespace rntizen::expo
