#include "FrameClock.h"

#include "../threading/MainThread.h"

#include <utility>

namespace rntizen {

namespace {

constexpr int kIdleFramesBeforeStop = 4;

using Owner = std::shared_ptr<FrameClock>;

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
    auto *owner = new Owner(self);
    if (ecore_animator_add(tick, owner) != nullptr) return;
    delete owner;
    std::lock_guard lock(self->mutex_);
    self->running_ = false;
  });
}

void FrameClock::close() {
  std::lock_guard lock(mutex_);
  closed_ = true;
}

Eina_Bool FrameClock::tick(void *data) {
  auto *owner = static_cast<Owner *>(data);
  auto &self = **owner;
  bool closed = false;
  bool requested = false;
  {
    std::lock_guard lock(self.mutex_);
    closed = self.closed_;
    requested = std::exchange(self.requested_, false);
  }
  const bool hadWork = !closed && (self.onFrame_() || requested);
  if (!self.shouldStop(hadWork)) return ECORE_CALLBACK_RENEW;
  delete owner;
  return ECORE_CALLBACK_CANCEL;
}

bool FrameClock::shouldStop(bool hadWork) {
  std::lock_guard lock(mutex_);
  idleFrames_ = hadWork ? 0 : idleFrames_ + 1;
  if (!closed_ && (requested_ || idleFrames_ < kIdleFramesBeforeStop)) return false;
  running_ = false;
  idleFrames_ = 0;
  return true;
}

} // namespace rntizen
