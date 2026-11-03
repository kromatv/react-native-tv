#include "ModalHostView.h"

#include "../mounting/ComponentViewRegistry.h"

#include <react/renderer/components/FBReactNativeSpec/EventEmitters.h>

#include <algorithm>
#include <vector>

namespace rntizen {

namespace {

std::vector<ModalHostView *> &mountedModals() {
  static std::vector<ModalHostView *> modals;
  return modals;
}

const facebook::react::ModalHostViewEventEmitter *modalEvents(const ComponentView &view) {
  const auto &emitter = view.eventEmitter();
  return emitter ? &static_cast<const facebook::react::ModalHostViewEventEmitter &>(*emitter) : nullptr;
}

} // namespace

ModalHostView::~ModalHostView() {
  leaveStack();
}

void ModalHostView::updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) {
  ComponentView::updateLayout(layoutMetrics);
  const auto *events = modalEvents(*this);
  if (shown_ || layoutMetrics.frame.size.width <= 0 || events == nullptr) return;
  shown_ = true;
  events->onShow({});
}

bool ModalHostView::requestCloseTopmost(Evas *evas) {
  const auto &modals = mountedModals();
  const auto topmost = std::find_if(modals.rbegin(), modals.rend(), [evas](auto *modal) { return modal->evas() == evas; });
  if (topmost == modals.rend()) return false;
  if (const auto *events = modalEvents(**topmost)) events->onRequestClose({});
  return true;
}

bool ModalHostView::isOverlay() const {
  return true;
}

void ModalHostView::didMount() {
  leaveStack();
  mountedModals().push_back(this);
}

void ModalHostView::willUnmount() {
  leaveStack();
}

void ModalHostView::leaveStack() {
  auto &modals = mountedModals();
  std::erase(modals, this);
}

RN_TIZEN_COMPONENT_VIEW("ModalHostView", ModalHostView);

} // namespace rntizen
