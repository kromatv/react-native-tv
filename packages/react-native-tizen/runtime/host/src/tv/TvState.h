#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// What the shell reports and JavaScript asks about the TV's own services:
/// Samsung's voice service, the Smart Hub preview and how the app was
/// launched. Thread-safe: the shell writes on the main thread, the
/// TurboModules read on the JavaScript thread.
class TvState final {
 public:
  /// Publishes Samsung's preview JSON through the shell; returns the channel
  /// that took it, 0 for none. Runs on the main thread.
  using PreviewPublisher = std::function<int(const std::string &json)>;

  /// The search URL template `expo.tizen.voice.searchUrl` set; nullopt without voice or a URL.
  explicit TvState(bool voiceConfigured, std::optional<std::string> searchUrl)
      : voiceConfigured_(voiceConfigured), searchUrl_(std::move(searchUrl)) {}

  bool voiceConfigured() const {
    return voiceConfigured_;
  }

  const std::optional<std::string> &searchUrl() const {
    return searchUrl_;
  }

  bool voiceAvailable() const {
    return voiceAvailable_.load();
  }

  void setVoiceAvailable(bool available) {
    voiceAvailable_ = available;
  }

  /// Whether JavaScript listens to `event`: `search`, or any other name for commands.
  bool listening(std::string_view event) const;
  void addListener(std::string_view event, int delta);

  /// The screen JavaScript said it shows; nullopt until it says.
  std::optional<std::string> screen() const {
    std::lock_guard lock(mutex_);
    return screen_;
  }

  void setScreen(std::optional<std::string> screen) {
    std::lock_guard lock(mutex_);
    screen_ = std::move(screen);
  }

  bool launchedFromTile() const {
    return launchedFromTile_.load();
  }

  void setLaunchedFromTile(bool fromTile) {
    launchedFromTile_ = fromTile;
  }

  PreviewPublisher previewPublisher() const {
    std::lock_guard lock(mutex_);
    return previewPublisher_;
  }

  void setPreviewPublisher(PreviewPublisher publisher) {
    std::lock_guard lock(mutex_);
    previewPublisher_ = std::move(publisher);
  }

 private:
  const bool voiceConfigured_;
  const std::optional<std::string> searchUrl_;
  std::atomic<bool> voiceAvailable_{false};
  std::atomic<int> searchListeners_{0};
  std::atomic<int> commandListeners_{0};
  std::atomic<bool> launchedFromTile_{false};
  mutable std::mutex mutex_;
  std::optional<std::string> screen_;
  PreviewPublisher previewPublisher_;
};

} // namespace rntizen
