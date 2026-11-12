#pragma once

#include "NinePatch.h"

#include <functional>

namespace rntizen {

/// The shadow patch for `corners` and `blur` (`makeShadowPatch`) when it has
/// been made already, else null. Main thread only.
const NinePatch *cachedShadowPatch(const Corners &corners, int blur);

/// Makes the shadow patch for `corners` and `blur` on a painting thread,
/// caches it, and calls `ready` with it on the main loop. Requests for a patch
/// already being made wait for the same one. Main thread only.
void requestShadowPatch(const Corners &corners, int blur, std::function<void(const NinePatch &)> ready);

} // namespace rntizen
