#include "AnimationFrameLoop.h"

#include "../logging/Log.h"
#include "../threading/MainThread.h"

#include <format>
#include <memory>
#include <utility>

namespace rnwebos {

AnimationFrameLoop::AnimationFrameLoop() = default;

AnimationFrameLoop::~AnimationFrameLoop() {
  if (frameCallback_ == 0) return;
  if (isMainThread()) {
    MainLoop::get().removeFrameCallback(frameCallback_);
  } else {
    runOnMainThread([id = frameCallback_] { MainLoop::get().removeFrameCallback(id); });
  }
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
  if (frameCallback_ != 0) return;
  frameCallback_ = MainLoop::get().addFrameCallback([weak = weak_from_this()](MainLoop::Clock::time_point) {
    if (auto self = weak.lock()) self->tick();
  });
  frames_ = 0;
  startedAt_ = std::chrono::steady_clock::now();
}

void AnimationFrameLoop::stopOnMainThread() {
  if (frameCallback_ != 0) {
    MainLoop::get().removeFrameCallback(std::exchange(frameCallback_, 0));
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt_).count();
    log(LogLevel::Debug, std::format("Native animated: {} frames in {} ms on the main loop", frames_, elapsed));
  }
  if (!ticking_) onFrame_ = nullptr;
}

void AnimationFrameLoop::tick() {
  if (frameCallback_ == 0) return;
  const auto owner = owner_.lock();
  if (!owner || !onFrame_) {
    stopOnMainThread();
    return;
  }
  ticking_ = true;
  ++frames_;
  auto onFrame = onFrame_;
  onFrame();
  ticking_ = false;
  if (frameCallback_ == 0) onFrame_ = nullptr;
}

} // namespace rnwebos
