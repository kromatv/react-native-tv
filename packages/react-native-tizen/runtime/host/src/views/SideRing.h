#pragma once

#include "BorderSides.h"
#include "NinePatch.h"

namespace rntizen {

/// The `part` of the border ring that `ringPatch` draws for the same `corners`
/// and `widths`: a white hollow nine-patch from the shared patch cache, its
/// corner pieces one pixel of straight border larger than the ring's. Main
/// thread only.
const NinePatch &sideRingPatch(const Corners &corners, const Edges &widths, const RingPart &part);

} // namespace rntizen
