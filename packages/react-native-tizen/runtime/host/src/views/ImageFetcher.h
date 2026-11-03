#pragma once

#include <react/renderer/imagemanager/primitives.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace rntizen {

/// An image's encoded bytes, or why there are none.
struct FetchedImage {
  std::shared_ptr<const std::string> bytes;
  std::string error;
  int status{0};
};

/// A fetch in flight. Dropping it drops the result.
class ImageFetch;

/// Fetches the bytes of an `http`, `https` or `data:` image without blocking
/// the main loop: downloads run on an HttpEngine of their own, and recent
/// downloads are kept in memory so a remounted poster appears at once. `done`
/// runs on the main loop while the returned fetch is alive. Main thread only.
std::shared_ptr<ImageFetch> fetchImage(const facebook::react::ImageSource &source, std::function<void(FetchedImage)> done);

/// Whether `uri` names something `fetchImage` fetches rather than a file.
bool isFetchedUri(std::string_view uri);

} // namespace rntizen
