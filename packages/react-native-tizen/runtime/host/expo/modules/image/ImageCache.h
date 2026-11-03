#pragma once

#include <optional>
#include <string>

namespace rntizen::expo {

/// An image file ready for Evas to decode, with expo-image's `cacheType`:
/// `none` when it was just downloaded, `disk` when it came from the cache.
struct ImageFile {
  std::string path;
  std::string cacheType;
};

/// The local file for the image at `uri`: a file URI's own path, or the cached
/// download keyed by `cacheKey` (the URI when empty), downloading it when it is
/// not cached. Blocks; throws std::runtime_error when the image cannot be had.
ImageFile imageFileFor(const std::string &uri, const std::string &cacheKey);

/// The cached file for `cacheKey`, if there is one.
std::optional<std::string> cachedImagePath(const std::string &cacheKey);

/// Deletes every cached download; returns whether there was a cache to delete.
bool clearImageDiskCache();

} // namespace rntizen::expo
