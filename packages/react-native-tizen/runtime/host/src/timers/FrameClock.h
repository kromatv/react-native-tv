#pragma once

#include <Ecore.h>

#include <functional>
#include <memory>
#include <mutex>

namespace rntizen {

/// Calls `onFrame` once per display frame, from an Ecore animator on the EFL
/// main loop, from the frame after `request` until `onFrame` has had nothing
/// to do for a few frames in a row. On a TV the animator ticks on the
/// compositor's frame callback, so this is the display's clock. `request` and
/// `close` may be called from any thread; after `close`, `onFrame` never runs
/// again.
class FrameClock final : public std::enable_shared_from_this<FrameClock> {
 public:
  /// `onFrame` runs on the main thread and returns whether it had work.
  explicit FrameClock(std::function<bool()> onFrame);

  FrameClock(const FrameClock &) = delete;
  FrameClock &operator=(const FrameClock &) = delete;

  void request();
  void close();

 private:
  static Eina_Bool tick(void *data);
  bool shouldStop(bool hadWork);

  const std::function<bool()> onFrame_;
  std::mutex mutex_;
  bool running_{false};
  bool requested_{false};
  bool closed_{false};
  int idleFrames_{0};
};

} // namespace rntizen
