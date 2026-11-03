#pragma once

#include <Evas.h>

namespace rntizen {

/// A clipped smart object whose color multiplies into its members', which is
/// how a view's opacity reaches its subtree. Its internal clipper is
/// unbounded, so it never crops a child. Moving it moves no member: each view
/// places its own objects and children in canvas pixels.
Evas_Object *addViewSmartObject(Evas *evas);

} // namespace rntizen
