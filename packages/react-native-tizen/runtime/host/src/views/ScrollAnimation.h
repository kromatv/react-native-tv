#pragma once

#include "../threading/FrameTicker.h"

#include <react/renderer/graphics/Point.h>

#include <functional>

namespace rntizen {

/// An animated scroll: moves an offset from one point to another over a
/// quarter of a second, decelerating, one step per drawn frame on the main
/// loop.
class ScrollAnimation final {
 public:
  using Step = std::function<void(facebook::react::Point offset, bool finished)>;

  ScrollAnimation() = default;

  ScrollAnimation(const ScrollAnimation &) = delete;
  ScrollAnimation &operator=(const ScrollAnimation &) = delete;

  /// Starts again from `from`, replacing any running animation.
  void start(facebook::react::Point from, facebook::react::Point to, Step step);
  void stop();
  bool isRunning() const;

 private:
  void onFrame();

  FrameTicker ticker_;
  facebook::react::Point from_;
  facebook::react::Point to_;
  Step step_;
  double startedAt_{0};
};

} // namespace rntizen
