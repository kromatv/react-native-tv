#include "../ExpoModuleRegistry.h"
#include "../FileIo.h"
#include "../ModuleDefinition.h"

#include "logging/Log.h"
#include "text/TextEngine.h"

#include <filesystem>
#include <mutex>
#include <set>

namespace rnwebos::expo {

namespace {

namespace fs = std::filesystem;

void registerFontFile(const std::string &family, const std::string &uri) {
  const auto source = localPathOf(uri);
  if (!source || !fs::is_regular_file(*source)) {
    throw CodedError("ERR_FONT_SOURCE", "No font file for " + family + " at " + uri);
  }
  if (registerAppFont(family, {fs::path(*source)}) == 0) {
    throw CodedError("ERR_FONT_FILE", "The file for " + family + " at " + uri + " is not a font");
  }
  log(LogLevel::Debug, "Loaded the font " + family + " from " + *source);
}

class ExpoFontLoaderModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoFontLoader")
        .function("getLoadedFonts", [this] { return loadedFonts(); })
        .function("isLoaded", [this](const std::string &family) {
          std::lock_guard lock(mutex_);
          return loaded_.contains(family);
        })
        .asyncFunction(
            "loadAsync",
            [this](const std::string &family, const std::string &localUri) {
              registerFontFile(family, localUri);
              markLoaded(family);
            })
        .asyncFunction(
            "loadFontFamilyAsync",
            [this](const std::string &family, const std::vector<folly::dynamic> &faces) {
              for (const auto &face : faces) {
                const auto *uri = face.get_ptr("localUri");
                if (uri == nullptr || !uri->isString()) throw CodedError("ERR_FONT_SOURCE", "A font face has no localUri");
                registerFontFile(family, uri->getString());
              }
              markLoaded(family);
            });
  }

 private:
  std::vector<std::string> loadedFonts() {
    std::lock_guard lock(mutex_);
    return {loaded_.begin(), loaded_.end()};
  }

  void markLoaded(const std::string &family) {
    std::lock_guard lock(mutex_);
    loaded_.insert(family);
  }

  std::mutex mutex_;
  std::set<std::string, std::less<>> loaded_;
};

} // namespace

RN_WEBOS_EXPO_MODULE(ExpoFontLoader, ExpoFontLoaderModule);

} // namespace rnwebos::expo
