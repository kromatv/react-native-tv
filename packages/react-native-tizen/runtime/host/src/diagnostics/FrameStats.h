#pragma once

#include <chrono>
#include <vector>

namespace rntizen {

/// Adds up one window of the main loop's frames from the events EFL reports:
/// animator ticks (the display's clock), canvas renders, and the loop waking
/// and going idle. A frame counts as dropped when the loop missed display
/// ticks and the canvas then rendered, so something was waiting to be drawn.
class FrameStats final {
 public:
  using Clock = std::chrono::steady_clock;

  struct Window {
    int frames{0};
    int ticks{0};
    int dropped{0};
    double periodMs{0};
    double renderAvgMs{0};
    double renderMaxMs{0};
    double mainBusyMs{0};
  };

  /// `fallbackPeriodMs` is the frame period to assume until ticks show the real one.
  explicit FrameStats(double fallbackPeriodMs);

  void tick(Clock::time_point now);
  void renderStarted(Clock::time_point now);
  void renderFinished(Clock::time_point now);
  void woke(Clock::time_point now);
  void sleeping(Clock::time_point now);

  /// The window so far, which `now` ends and the next one starts at.
  Window take(Clock::time_point now);

 private:
  double periodMs_;
  Window window_;
  std::vector<double> gaps_;
  Clock::time_point lastTick_{};
  Clock::time_point renderStart_{};
  Clock::time_point wokeAt_{};
  double renderTotalMs_{0};
  int stalledTicks_{0};
  bool renderedSinceTick_{false};
  bool awake_{false};
};

} // namespace rntizen
