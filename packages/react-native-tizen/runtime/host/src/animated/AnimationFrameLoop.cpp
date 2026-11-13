#include "AnimationFrameLoop.h"

#include "../logging/Log.h"
#include "../threading/MainThread.h"

#include <format>
#include <memory>
#include <utility>

namespace rntizen {

AnimationFrameLoop::AnimationFrameLoop() = default;

AnimationFrameLoop::~AnimationFrameLoop() {
  if (isMainThread()) return;
  runOnMainThread([ticker = std::move(ticker_)] { ticker->stop(); });
}

void AnimationFrameLoop::follow(std::weak_ptr<const void> owner) {
  owner_ = std::move(owner);
}

void AnimationFrameLoop::start(std::function<void()> onFrame) {
  if (isMainThread()) {
    startOnMainThread(std::move(onFrame));
    return;
  }
  runOnMainThread([self = shared_from_this(), onFrame = std::move(onFrame)]() mutable {
    self->startOnMainThread(std::move(onFrame));
  });
}

void AnimationFrameLoop::stop() {
  if (isMainThread()) {
    stopOnMainThread();
    return;
  }
  runOnMainThread([self = shared_from_this()] { self->stopOnMainThread(); });
}

void AnimationFrameLoop::startOnMainThread(std::function<void()> onFrame) {
  onFrame_ = std::move(onFrame);
  if (ticker_->isRunning()) return;
  ticker_->start([weak = weak_from_this()] {
    if (const auto self = weak.lock()) self->frame();
  });
  frames_ = 0;
  startedAt_ = std::chrono::steady_clock::now();
}

void AnimationFrameLoop::stopOnMainThread() {
  if (ticker_->isRunning()) {
    ticker_->stop();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt_).count();
    log(LogLevel::Debug, std::format("Native animated: {} frames in {} ms on the main loop", frames_, elapsed));
  }
  if (!ticking_) onFrame_ = nullptr;
}

void AnimationFrameLoop::frame() {
  if (const auto owner = owner_.lock(); owner && onFrame_) {
    ticking_ = true;
    ++frames_;
    auto onFrame = onFrame_;
    onFrame();
    ticking_ = false;
    if (!ticker_->isRunning()) onFrame_ = nullptr;
    return;
  }
  stopOnMainThread();
}

} // namespace rntizen
