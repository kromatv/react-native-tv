#include "ScrollAnimation.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>

namespace rnwebos {

namespace {

constexpr std::chrono::duration<double> kDuration{0.25};

} // namespace

ScrollAnimation::~ScrollAnimation() {
  stop();
}

void ScrollAnimation::start(facebook::react::Point from, facebook::react::Point to, Step step) {
  stop();
  from_ = from;
  to_ = to;
  step_ = std::move(step);
  started_ = MainLoop::Clock::now();
  frameCallback_ = MainLoop::get().addFrameCallback([this](MainLoop::Clock::time_point frameTime) { onFrame(frameTime); });
}

void ScrollAnimation::stop() {
  if (frameCallback_ == 0) return;
  MainLoop::get().removeFrameCallback(frameCallback_);
  frameCallback_ = 0;
}

bool ScrollAnimation::isRunning() const {
  return frameCallback_ != 0;
}

facebook::react::Point ScrollAnimation::offsetAt(facebook::react::Point from, facebook::react::Point to, double position) {
  const auto progress =
      static_cast<facebook::react::Float>(std::sin(std::clamp(position, 0.0, 1.0) * std::numbers::pi / 2));
  return {.x = from.x + (to.x - from.x) * progress, .y = from.y + (to.y - from.y) * progress};
}

void ScrollAnimation::onFrame(MainLoop::Clock::time_point frameTime) {
  const auto position = std::chrono::duration<double>(frameTime - started_) / kDuration;
  const bool finished = position >= 1.0;
  const auto offset = finished ? to_ : offsetAt(from_, to_, position);
  if (finished) stop();
  const auto step = step_;
  step(offset, finished);
}

} // namespace rnwebos
