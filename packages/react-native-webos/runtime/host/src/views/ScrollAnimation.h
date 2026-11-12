#pragma once

#include "../platform/MainLoop.h"

#include <react/renderer/graphics/Point.h>

#include <functional>

namespace rnwebos {

/// An animated scroll: moves an offset from one point to another over a
/// quarter of a second, decelerating, one step per frame on the main loop.
class ScrollAnimation final {
 public:
  using Step = std::function<void(facebook::react::Point offset, bool finished)>;

  ScrollAnimation() = default;
  ~ScrollAnimation();

  ScrollAnimation(const ScrollAnimation &) = delete;
  ScrollAnimation &operator=(const ScrollAnimation &) = delete;

  /// Starts again from `from`, replacing any running animation.
  void start(facebook::react::Point from, facebook::react::Point to, Step step);
  void stop();
  bool isRunning() const;

  /// Where an animation from `from` to `to` is once `position` of its time
  /// has passed, from 0 to 1.
  static facebook::react::Point
  offsetAt(facebook::react::Point from, facebook::react::Point to, double position);

 private:
  void onFrame(MainLoop::Clock::time_point frameTime);

  MainLoop::Id frameCallback_{0};
  MainLoop::Clock::time_point started_;
  facebook::react::Point from_;
  facebook::react::Point to_;
  Step step_;
};

} // namespace rnwebos
