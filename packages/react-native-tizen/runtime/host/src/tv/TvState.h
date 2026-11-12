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
  explicit TvState(bool voiceConfigured, std::optional<std::string> searchUrl);

  bool voiceConfigured() const;
  const std::optional<std::string> &searchUrl() const;

  bool voiceAvailable() const;
  void setVoiceAvailable(bool available);

  /// Whether JavaScript listens to `event`: `search`, or any other name for commands.
  bool listening(std::string_view event) const;
  void addListener(std::string_view event, int delta);

  /// The screen JavaScript said it shows; nullopt until it says.
  std::optional<std::string> screen() const;
  void setScreen(std::optional<std::string> screen);

  bool launchedFromTile() const;
  void setLaunchedFromTile(bool fromTile);

  PreviewPublisher previewPublisher() const;
  void setPreviewPublisher(PreviewPublisher publisher);

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
