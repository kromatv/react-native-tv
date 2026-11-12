#include "FrameStats.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rntizen {

namespace {

constexpr double kMinPeriodMs = 4;
constexpr double kMaxPeriodMs = 50;
constexpr std::size_t kMinGapsForPeriod = 5;

double millis(FrameStats::Clock::duration duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

} // namespace

FrameStats::FrameStats(double fallbackPeriodMs) : periodMs_(std::clamp(fallbackPeriodMs, kMinPeriodMs, kMaxPeriodMs)) {}

void FrameStats::tick(Clock::time_point now) {
  ++window_.ticks;
  if (lastTick_ != Clock::time_point{}) {
    const double gap = millis(now - lastTick_);
    gaps_.push_back(gap);
    if (stalledTicks_ > 0 && renderedSinceTick_) window_.dropped += stalledTicks_;
    stalledTicks_ = std::max(0, static_cast<int>(std::lround(gap / periodMs_)) - 1);
  }
  renderedSinceTick_ = false;
  lastTick_ = now;
}

void FrameStats::renderStarted(Clock::time_point now) {
  renderStart_ = now;
}

void FrameStats::renderFinished(Clock::time_point now) {
  if (renderStart_ == Clock::time_point{}) return;
  const double duration = millis(now - std::exchange(renderStart_, Clock::time_point{}));
  ++window_.frames;
  renderTotalMs_ += duration;
  window_.renderMaxMs = std::max(window_.renderMaxMs, duration);
  renderedSinceTick_ = true;
}

void FrameStats::woke(Clock::time_point now) {
  awake_ = true;
  wokeAt_ = now;
}

void FrameStats::sleeping(Clock::time_point now) {
  if (!awake_) return;
  window_.mainBusyMs += millis(now - wokeAt_);
  awake_ = false;
}

FrameStats::Window FrameStats::take(Clock::time_point now) {
  if (awake_) {
    window_.mainBusyMs += millis(now - wokeAt_);
    wokeAt_ = now;
  }
  if (gaps_.size() >= kMinGapsForPeriod) {
    auto middle = gaps_.begin() + static_cast<std::ptrdiff_t>(gaps_.size() / 2);
    std::ranges::nth_element(gaps_, middle);
    periodMs_ = std::clamp(*middle, kMinPeriodMs, kMaxPeriodMs);
  }
  auto finished = std::exchange(window_, Window{});
  finished.periodMs = periodMs_;
  finished.renderAvgMs = finished.frames > 0 ? renderTotalMs_ / finished.frames : 0;
  renderTotalMs_ = 0;
  gaps_.clear();
  return finished;
}

} // namespace rntizen
