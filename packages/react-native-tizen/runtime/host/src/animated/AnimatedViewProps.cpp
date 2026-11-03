#include "AnimatedViewProps.h"

#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>

namespace rntizen {

void applyAnimatedProps(
    MountingManager &mountingManager,
    const facebook::react::Scheduler &scheduler,
    const facebook::react::ContextContainer &contextContainer,
    facebook::react::SurfaceId surfaceId,
    facebook::react::Tag tag,
    const folly::dynamic &props) {
  using namespace facebook::react;
  auto *view = mountingManager.findView(tag);
  const auto handle = mountingManager.componentHandle(tag);
  if (view == nullptr || !view->props() || !handle.has_value()) return;
  const auto *descriptor = scheduler.findComponentDescriptorByHandle_DO_NOT_USE_THIS_IS_BROKEN(*handle);
  if (descriptor == nullptr) return;
  view->updateProps(
      descriptor->cloneProps(PropsParserContext{surfaceId, contextContainer}, view->props(), RawProps(props)));
}

} // namespace rntizen
