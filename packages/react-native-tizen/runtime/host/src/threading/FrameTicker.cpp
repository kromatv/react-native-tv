#include "FrameTicker.h"

#include <utility>

namespace rntizen {

FrameTicker::~FrameTicker() {
  stop();
}

void FrameTicker::start(std::function<void()> onFrame) {
  onFrame_ = std::move(onFrame);
  if (animator_ != nullptr) return;
  ticked_ = false;
  animator_ = ecore_animator_add(onTick, this);
  // Ecore_Evas renders from an idle enterer of its own (efl-rendering.md
  // section 1): ahead of it, this turn's frame shows the step.
  beforeRender_ = ecore_idle_enterer_before_add(onBeforeRender, this);
}

void FrameTicker::stop() {
  if (animator_ == nullptr) return;
  ecore_animator_del(std::exchange(animator_, nullptr));
  ecore_idle_enterer_del(std::exchange(beforeRender_, nullptr));
  onFrame_ = nullptr;
}

bool FrameTicker::isRunning() const {
  return animator_ != nullptr;
}

Eina_Bool FrameTicker::onTick(void *data) {
  static_cast<FrameTicker *>(data)->ticked_ = true;
  return ECORE_CALLBACK_RENEW;
}

Eina_Bool FrameTicker::onBeforeRender(void *data) {
  auto &self = *static_cast<FrameTicker *>(data);
  if (!std::exchange(self.ticked_, false)) return ECORE_CALLBACK_RENEW;
  const auto onFrame = self.onFrame_;
  if (onFrame) onFrame();
  return ECORE_CALLBACK_RENEW;
}

} // namespace rntizen
