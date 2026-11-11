#pragma once

#include <mutex>
#include <optional>
#include <string>

namespace rntizen {

/// The URI of the app control that launched the app, what `Linking.getInitialURL`
/// answers. The host sets it on the main thread; `LinkingManager` reads it on
/// the JavaScript thread.
class LaunchUrl final {
 public:
  void set(std::string url) {
    std::lock_guard lock(mutex_);
    url_ = std::move(url);
  }

  std::optional<std::string> get() const {
    std::lock_guard lock(mutex_);
    return url_;
  }

 private:
  mutable std::mutex mutex_;
  std::optional<std::string> url_;
};

} // namespace rntizen
