#include "FileSystemFile.h"

#include "../../AppDirectories.h"
#include "../../ExpoModuleRegistry.h"
#include "../../FileIo.h"

#include "network/HttpGet.h"

#include <sys/statvfs.h>

#include <filesystem>
#include <string_view>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

double diskSpace(bool available) {
  struct statvfs stats {};
  if (statvfs(appDirectories().data.c_str(), &stats) != 0) return 0;
  const auto blocks = available ? stats.f_bavail : stats.f_blocks;
  return static_cast<double>(blocks) * static_cast<double>(stats.f_frsize);
}

std::string fileNameOf(std::string_view url) {
  const auto path = url.substr(0, url.find_first_of("?#"));
  const auto slash = path.find_last_of('/');
  const auto name = slash == std::string_view::npos ? path : path.substr(slash + 1);
  return name.empty() ? "download" : std::string(name);
}

std::string download(const std::string &url, const FileSystemPath &destination, const std::optional<folly::dynamic> &options) {
  const auto target = destination.isDirectory() ? (fs::path(destination.path()) / fileNameOf(url)) : fs::path(destination.path());
  if (fs::exists(target) && !optionFlag(options, "idempotent")) {
    throw CodedError("ERR_DESTINATION_EXISTS", "Destination '" + target.string() + "' already exists");
  }
  writeFileAtomically(target, httpGet(url));
  return FileSystemFile(fileUri(target.string())).uri();
}

class FileSystemModule final : public Module {
 public:
  ModuleDefinition definition() override {
    const auto &directories = appDirectories();
    const auto bundle = context().resourceDirectory();
    return ModuleDefinition("FileSystem")
        .constant("documentDirectory", [&directories] { return fileUri(directories.data); })
        .constant("cacheDirectory", [&directories] { return fileUri(directories.cache); })
        .constant("bundleDirectory", [bundle] { return bundle; })
        .property("totalDiskSpace", [] { return diskSpace(false); })
        .property("availableDiskSpace", [] { return diskSpace(true); })
        .function("info", [](const std::string &uri) { return pathInfo(uri); })
        .asyncFunction(
            "downloadFileAsync",
            [](const std::string &url,
               const std::shared_ptr<FileSystemPath> &destination,
               const std::optional<folly::dynamic> &options,
               const std::optional<std::string> &) { return download(url, *destination, options); })
        .function("cancelDownloadAsync", [](const std::string &) {})
        .sharedClass(fileClass())
        .sharedClass(directoryClass());
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(FileSystem, FileSystemModule);

} // namespace rntizen::expo
