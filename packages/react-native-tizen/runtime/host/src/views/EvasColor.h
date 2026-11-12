#pragma once

#include <react/renderer/graphics/Color.h>

#include <Evas.h>

namespace rntizen {

/// Sets `object`'s color from a React Native color, multiplied by `opacity` and
/// premultiplied as Evas expects. A null color is transparent.
void setColor(Evas_Object *object, const facebook::react::SharedColor &color, float opacity = 1);

/// Whether `color` paints anything.
bool isVisible(const facebook::react::SharedColor &color);

} // namespace rntizen
