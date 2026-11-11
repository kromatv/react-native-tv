#include "FrameClock.h"

#include "../logging/Log.h"
#include "../threading/MainThread.h"

#include <utility>

namespace rnwebos {

namespace {

constexpr int kIdleFramesBeforeStop = 4;

} // namespace

FrameClock::FrameClock(std::function<bool()> onFrame) : onFrame_(std::move(onFrame)) {}

void FrameClock::request() {
  {
    std::lock_guard lock(mutex_);
    if (closed_) return;
    requested_ = true;
    if (running_) return;
    running_ = true;
  }
  runOnMainThread([self = shared_from_this()] {
    auto id = std::make_shared<MainLoop::Id>(0);
    *id = MainLoop::get().addFrameCallback([self, id](MainLoop::Clock::time_point) { self->tick(*id); });
  });
}

void FrameClock::close() {
  std::lock_guard lock(mutex_);
  closed_ = true;
}

bool FrameClock::runFrame() {
  try {
    return onFrame_();
  } catch (const std::exception &error) {
    logUncaughtException("a frame", error);
  } catch (...) {
    logUncaughtException("a frame");
  }
  return false;
}

void FrameClock::tick(MainLoop::Id id) {
  bool closed = false;
  bool requested = false;
  {
    std::lock_guard lock(mutex_);
    closed = closed_;
    requested = std::exchange(requested_, false);
  }
  const bool hadWork = !closed && (runFrame() || requested);
  if (shouldStop(hadWork)) MainLoop::get().removeFrameCallback(id);
}

bool FrameClock::shouldStop(bool hadWork) {
  std::lock_guard lock(mutex_);
  idleFrames_ = hadWork ? 0 : idleFrames_ + 1;
  if (!closed_ && (requested_ || idleFrames_ < kIdleFramesBeforeStop)) return false;
  running_ = false;
  idleFrames_ = 0;
  return true;
}

} // namespace rnwebos
