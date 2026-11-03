#include "AppDirectories.h"

#include <app_common.h>

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace rntizen::expo {

namespace {

std::string withTrailingSlash(std::string path) {
  if (!path.empty() && path.back() != '/') path += '/';
  return path;
}

std::string takePath(char *path) {
  if (path == nullptr) return {};
  std::string result = path;
  std::free(path);
  return withTrailingSlash(std::move(result));
}

std::string writablePath(char *path, const char *fallback) {
  auto result = takePath(path);
  return result.empty() ? withTrailingSlash((std::filesystem::temp_directory_path() / fallback).string()) : result;
}

AppDirectories readDirectories() {
  AppDirectories directories{
      .data = writablePath(app_get_data_path(), "rn-tizen-data"),
      .cache = writablePath(app_get_cache_path(), "rn-tizen-cache"),
      .resource = takePath(app_get_resource_path()),
  };
  std::error_code ignored;
  std::filesystem::create_directories(directories.data, ignored);
  std::filesystem::create_directories(directories.cache, ignored);
  return directories;
}

} // namespace

const AppDirectories &appDirectories() {
  static const AppDirectories directories = readDirectories();
  return directories;
}

std::string fileUri(const std::string &path) {
  return "file://" + path;
}

} // namespace rntizen::expo
