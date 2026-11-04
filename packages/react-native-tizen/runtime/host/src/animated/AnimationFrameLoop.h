#pragma once

#include <Ecore.h>

#include <chrono>
#include <functional>
#include <memory>

namespace rntizen {

/// The frame clock React Native's C++ animated driver asks its platform for
/// (`NativeAnimatedNodesManager`'s start and stop callbacks): runs a callback
/// on every frame of the EFL main loop from an Ecore animator, as iOS runs it
/// from a CADisplayLink. `start` and `stop` may be called from any thread; the
/// callback runs on the main thread, and never once the owner given to
/// `follow` has died, so a frame queued during teardown cannot reach a
/// destroyed driver.
class AnimationFrameLoop final : public std::enable_shared_from_this<AnimationFrameLoop> {
 public:
  AnimationFrameLoop();
  ~AnimationFrameLoop();

  AnimationFrameLoop(const AnimationFrameLoop &) = delete;
  AnimationFrameLoop &operator=(const AnimationFrameLoop &) = delete;

  /// Ties the loop to `owner`, what the callback reaches into. Call it once,
  /// before the first `start`.
  void follow(std::weak_ptr<const void> owner);

  void start(std::function<void()> onFrame);
  void stop();

 private:
  static Eina_Bool tick(void *data);
  void startOnMainThread(std::function<void()> onFrame);
  void stopOnMainThread();

  std::weak_ptr<const void> owner_;
  Ecore_Animator *animator_{nullptr};
  std::function<void()> onFrame_;
  bool ticking_{false};
  int frames_{0};
  std::chrono::steady_clock::time_point startedAt_;
};

} // namespace rntizen
