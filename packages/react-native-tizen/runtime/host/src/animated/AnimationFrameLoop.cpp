#include "AnimationFrameLoop.h"

#include "../logging/Log.h"
#include "../threading/MainThread.h"

#include <utility>

namespace rntizen {

namespace {

using WeakLoop = std::weak_ptr<AnimationFrameLoop>;

void deleteAnimator(Ecore_Animator *animator) {
  delete static_cast<WeakLoop *>(ecore_animator_del(animator));
}

} // namespace

AnimationFrameLoop::~AnimationFrameLoop() {
  if (animator_ == nullptr) return;
  if (isMainThread()) {
    deleteAnimator(animator_);
  } else {
    runOnMainThread([animator = animator_] { deleteAnimator(animator); });
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
  if (animator_ != nullptr) return;
  animator_ = ecore_animator_add(tick, new WeakLoop(weak_from_this()));
  frames_ = 0;
  startedAt_ = std::chrono::steady_clock::now();
}

void AnimationFrameLoop::stopOnMainThread() {
  if (animator_ != nullptr) {
    deleteAnimator(std::exchange(animator_, nullptr));
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt_).count();
    log(LogLevel::Debug,
        "Native animated: " + std::to_string(frames_) + " frames in " + std::to_string(elapsed) +
            " ms on the main loop");
  }
  if (!ticking_) onFrame_ = nullptr;
}

Eina_Bool AnimationFrameLoop::tick(void *data) {
  auto self = static_cast<WeakLoop *>(data)->lock();
  if (!self || self->animator_ == nullptr) return ECORE_CALLBACK_RENEW;
  const auto owner = self->owner_.lock();
  if (!owner || !self->onFrame_) {
    self->stopOnMainThread();
    return ECORE_CALLBACK_RENEW;
  }
  self->ticking_ = true;
  ++self->frames_;
  auto onFrame = self->onFrame_;
  onFrame();
  self->ticking_ = false;
  if (self->animator_ == nullptr) self->onFrame_ = nullptr;
  return ECORE_CALLBACK_RENEW;
}

} // namespace rntizen
