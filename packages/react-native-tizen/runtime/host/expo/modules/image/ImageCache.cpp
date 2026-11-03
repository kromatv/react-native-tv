#include "ImageCache.h"

#include "../../AppDirectories.h"
#include "../../FileIo.h"

#include "network/Digest.h"
#include "network/HttpGet.h"

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string_view>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

class ImageSourceError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

bool isRemote(std::string_view uri) {
  const auto end = uri.find("://");
  if (end == std::string_view::npos) return false;
  const auto scheme = uri.substr(0, end);
  return scheme == "http" || scheme == "https";
}

fs::path cacheDirectory() {
  return fs::path(appDirectories().cache) / "ExpoImage";
}

fs::path cachePath(const std::string &cacheKey) {
  return cacheDirectory() / md5Hex(cacheKey);
}

} // namespace

ImageFile imageFileFor(const std::string &uri, const std::string &cacheKey) {
  if (auto local = localPathOf(uri)) {
    if (!fs::is_regular_file(*local)) throw ImageSourceError(std::format("No image file at {}", uri));
    return {.path = *local, .cacheType = "disk"};
  }
  if (!isRemote(uri)) throw ImageSourceError(std::format("Unsupported image source {}", uri));
  const auto path = cachePath(cacheKey.empty() ? uri : cacheKey);
  std::error_code error;
  if (fs::is_regular_file(path, error) && fs::file_size(path, error) > 0) return {.path = path.string(), .cacheType = "disk"};
  fs::create_directories(cacheDirectory(), error);
  writeFileAtomically(path, httpGet(uri));
  return {.path = path.string(), .cacheType = "none"};
}

std::optional<std::string> cachedImagePath(const std::string &cacheKey) {
  const auto path = cachePath(cacheKey);
  if (std::error_code error; !fs::is_regular_file(path, error)) return std::nullopt;
  return path.string();
}

bool clearImageDiskCache() {
  std::error_code error;
  return fs::remove_all(cacheDirectory(), error) > 0;
}

} // namespace rntizen::expo
