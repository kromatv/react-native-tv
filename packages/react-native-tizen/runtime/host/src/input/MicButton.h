#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string_view>

namespace rntizen {

/// The Smart Remote's microphone key, which reaches an app ungrabbed and opens
/// Samsung's assistant unless the app grabs it (tizen-input.md, section 9).
inline constexpr std::string_view kMicButtonKey = "XF86BTVoice";

/// The Smart Remote's microphone button, held by the app while JavaScript
/// listens to it: the grab keeps Samsung's assistant closed, and the button's
/// down and up reach JavaScript as they happen, as `pressIn` and `pressOut`.
class MicButton final : public std::enable_shared_from_this<MicButton> {
 public:
  /// Grabs or releases the key on the app's window. Main thread.
  using Grab = std::function<void(bool grabbed)>;
  /// Sends `pressIn` or `pressOut` to JavaScript. Any thread.
  using Emit = std::function<void(std::string_view type)>;
  /// Runs a task on the main thread.
  using Post = std::function<void(std::function<void()> task)>;

  MicButton(Emit emit, Post post);

  MicButton(const MicButton &) = delete;
  MicButton &operator=(const MicButton &) = delete;

  /// Main thread: from now on grabs and releases the key with `grab`.
  void setGrab(Grab grab);
  /// Any thread: one more or one less JavaScript listener.
  void claim();
  void unclaim();
  /// Main thread: whether the key event is the held button's, which then goes nowhere else.
  bool handleKey(std::string_view keyName, bool isDown);
  /// Main thread: a held button ends with `pressOut`, for an app that will not see the release.
  void releaseHeld();
  /// Main thread: forgets every listener, for a reload that drops them all.
  void reset();

 private:
  void apply();
  void applyLater();

  const Emit emit_;
  const Post post_;
  Grab grab_;
  std::atomic<int> claims_{0};
  bool grabbed_{false};
  bool held_{false};
};

} // namespace rntizen
