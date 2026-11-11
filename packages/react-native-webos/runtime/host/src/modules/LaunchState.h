#pragma once

#include <folly/dynamic.h>

#include <mutex>
#include <optional>
#include <string>

namespace rnwebos {

/// How the TV's app manager launched the app, and launched it again since. The
/// host sets it on the main thread; modules read it on the JavaScript thread.
class LaunchState final {
 public:
  /// The deep link the app was launched with, which `Linking.getInitialURL` answers.
  void setInitialUrl(std::string url) {
    std::lock_guard lock(mutex_);
    initialUrl_ = std::move(url);
  }

  std::optional<std::string> initialUrl() const {
    std::lock_guard lock(mutex_);
    return initialUrl_;
  }

  /// The parameters of the launch, then of each relaunch.
  void setParameters(folly::dynamic parameters) {
    std::lock_guard lock(mutex_);
    parameters_ = std::move(parameters);
  }

  folly::dynamic parameters() const {
    std::lock_guard lock(mutex_);
    return parameters_;
  }

 private:
  mutable std::mutex mutex_;
  std::optional<std::string> initialUrl_;
  folly::dynamic parameters_ = folly::dynamic::object();
};

} // namespace rnwebos
