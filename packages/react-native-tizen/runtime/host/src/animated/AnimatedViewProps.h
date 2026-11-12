#pragma once

#include "../mounting/MountingManager.h"

#include <folly/dynamic.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <react/utils/ContextContainer.h>

namespace rntizen {

/// Applies the props the native animated driver computed for `tag` (opacity,
/// transform...) to its mounted view, now and without a commit: the view's
/// props cloned with them through its component descriptor, as iOS's
/// `RCTMountingManager synchronouslyUpdateViewOnUIThread` does. A view that is
/// not mounted is skipped. Main thread only.
void applyAnimatedProps(
    MountingManager &mountingManager,
    const facebook::react::Scheduler &scheduler,
    const facebook::react::ContextContainer &contextContainer,
    facebook::react::SurfaceId surfaceId,
    facebook::react::Tag tag,
    const folly::dynamic &props);

} // namespace rntizen
