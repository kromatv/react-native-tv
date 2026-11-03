#include "../AppDirectories.h"
#include "../ExpoModuleRegistry.h"
#include "../FileIo.h"
#include "../ModuleDefinition.h"

#include "logging/Log.h"
#include "network/Digest.h"
#include "text/AppFonts.h"
#include "text/FontInstances.h"
#include "threading/MainThread.h"

#include <Evas.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <set>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

std::string contentsOf(const fs::path &file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// fontconfig scans a directory when it is appended, so each font gets a
// directory of its own and is in it before Evas learns the path. A variable
// font is registered as its static weights only, never beside them.
void registerFontFile(const std::string &family, const std::string &uri) {
  const auto source = localPathOf(uri);
  if (!source || !fs::is_regular_file(*source)) {
    throw CodedError("ERR_FONT_SOURCE", "No font file for " + family + " at " + uri);
  }
  const auto key = md5Hex(family + "\n" + md5Hex(contentsOf(*source)));
  const auto directory = fs::path(appDirectories().cache) / "ExpoFontLoader" / key;
  fs::create_directories(directory);
  auto files = writeWeightInstances(*source, directory);
  if (files.empty()) {
    files.push_back(directory / fs::path(*source).filename());
    fs::copy_file(*source, files.back(), fs::copy_options::overwrite_existing);
  }
  registerAppFontFiles(family, files);
  runOnMainThreadSync([&directory] { evas_font_path_global_append(directory.c_str()); });
  log(LogLevel::Debug, "Loaded the font " + family + " from " + directory.string());
}

class ExpoFontLoaderModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoFontLoader")
        .function("getLoadedFonts", [this] { return loadedFonts(); })
        .function("isLoaded", [this](std::string family) {
          std::lock_guard lock(mutex_);
          return loaded_.contains(family);
        })
        .asyncFunction(
            "loadAsync",
            [this](std::string family, std::string localUri) {
              registerFontFile(family, localUri);
              markLoaded(family);
            })
        .asyncFunction(
            "loadFontFamilyAsync",
            [this](std::string family, std::vector<folly::dynamic> faces) {
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
  std::set<std::string> loaded_;
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoFontLoader, ExpoFontLoaderModule);

} // namespace rntizen::expo
