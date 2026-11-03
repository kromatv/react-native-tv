#include "../AppDirectories.h"
#include "../ExpoModuleRegistry.h"
#include "../FileIo.h"
#include "../ModuleDefinition.h"

#include "network/Digest.h"
#include "network/HttpGet.h"

namespace rntizen::expo {

namespace {

std::string downloadAsset(const std::string &url, const std::optional<std::string> &md5Hash, const std::string &type) {
  if (url.rfind("file://", 0) == 0) return url;
  if (!url.empty() && url.front() == '/') return fileUri(url);
  const auto key = md5Hash.value_or(md5Hex(url));
  const auto path = std::filesystem::path(appDirectories().cache) / ("ExponentAsset-" + key + "." + type);
  if (auto cached = readFile(path); cached && (!md5Hash || md5Hex(*cached) == *md5Hash)) return fileUri(path.string());
  writeFileAtomically(path, httpGet(url));
  return fileUri(path.string());
}

class ExpoAssetModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoAsset")
        .asyncFunction("downloadAsync", [](std::string url, std::optional<std::string> md5Hash, std::optional<std::string> type) {
          return downloadAsset(url, md5Hash, type.value_or(""));
        });
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoAsset, ExpoAssetModule);

} // namespace rntizen::expo
