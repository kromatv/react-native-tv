#pragma once

#include "../platform/MainLoop.h"

#include <functional>
#include <memory>
#include <mutex>

namespace rnwebos {

/// Calls `onFrame` once per display frame, on the main loop, from the frame
/// after `request` until `onFrame` has had nothing to do for a few frames in a
/// row. `request` and `close` may be called from any thread; after `close`,
/// `onFrame` never runs again.
class FrameClock final : public std::enable_shared_from_this<FrameClock> {
 public:
  /// `onFrame` runs on the main thread and returns whether it had work.
  explicit FrameClock(std::function<bool()> onFrame);

  FrameClock(const FrameClock &) = delete;
  FrameClock &operator=(const FrameClock &) = delete;

  void request();
  void close();

 private:
  void tick(MainLoop::Id id);
  bool runFrame();
  bool shouldStop(bool hadWork);

  const std::function<bool()> onFrame_;
  std::mutex mutex_;
  bool running_{false};
  bool requested_{false};
  bool closed_{false};
  int idleFrames_{0};
};

} // namespace rnwebos
