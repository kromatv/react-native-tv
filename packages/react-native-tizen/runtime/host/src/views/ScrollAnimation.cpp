#include "ScrollAnimation.h"

namespace rntizen {

namespace {

constexpr double kDurationSeconds = 0.25;

} // namespace

ScrollAnimation::~ScrollAnimation() {
  stop();
}

void ScrollAnimation::start(facebook::react::Point from, facebook::react::Point to, Step step) {
  stop();
  from_ = from;
  to_ = to;
  step_ = std::move(step);
  animator_ = ecore_animator_timeline_add(kDurationSeconds, onFrame, this);
}

void ScrollAnimation::stop() {
  if (animator_ == nullptr) return;
  ecore_animator_del(animator_);
  animator_ = nullptr;
}

bool ScrollAnimation::isRunning() const {
  return animator_ != nullptr;
}

Eina_Bool ScrollAnimation::onFrame(void *data, double position) {
  auto &self = *static_cast<ScrollAnimation *>(data);
  const auto progress = static_cast<facebook::react::Float>(ecore_animator_pos_map(position, ECORE_POS_MAP_DECELERATE, 0, 0));
  const bool finished = position >= 1.0;
  const facebook::react::Point offset{
      .x = self.from_.x + (self.to_.x - self.from_.x) * progress,
      .y = self.from_.y + (self.to_.y - self.from_.y) * progress,
  };
  if (finished) self.animator_ = nullptr;
  auto step = self.step_;
  step(offset, finished);
  return finished ? ECORE_CALLBACK_CANCEL : ECORE_CALLBACK_RENEW;
}

} // namespace rntizen
