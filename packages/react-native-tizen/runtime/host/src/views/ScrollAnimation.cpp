#include "ScrollAnimation.h"

#include <algorithm>

namespace rntizen {

namespace {

constexpr double kDurationSeconds = 0.25;

} // namespace

void ScrollAnimation::start(facebook::react::Point from, facebook::react::Point to, Step step) {
  from_ = from;
  to_ = to;
  step_ = std::move(step);
  startedAt_ = ecore_loop_time_get();
  ticker_.start([this] { onFrame(); });
}

void ScrollAnimation::stop() {
  ticker_.stop();
}

bool ScrollAnimation::isRunning() const {
  return ticker_.isRunning();
}

void ScrollAnimation::onFrame() {
  const double position = std::clamp((ecore_loop_time_get() - startedAt_) / kDurationSeconds, 0.0, 1.0);
  const auto progress = static_cast<facebook::react::Float>(ecore_animator_pos_map(position, ECORE_POS_MAP_DECELERATE, 0, 0));
  const bool finished = position >= 1.0;
  const facebook::react::Point offset{
      .x = from_.x + (to_.x - from_.x) * progress,
      .y = from_.y + (to_.y - from_.y) * progress,
  };
  const auto step = step_;
  if (finished) ticker_.stop();
  step(offset, finished);
}

} // namespace rntizen
