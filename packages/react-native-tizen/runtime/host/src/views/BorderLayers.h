#pragma once

#include "BorderSides.h"
#include "NinePatch.h"

#include <react/renderer/components/view/primitives.h>
#include <react/renderer/graphics/Color.h>

#include <vector>

namespace rntizen {

/// One image of a border: the part of the ring it draws, and the color that
/// tints it.
struct BorderLayer {
  RingPart part;
  facebook::react::SharedColor color;

  bool operator==(const BorderLayer &) const = default;
};

/// The images that draw a border of `widths` and `colors`, lowest first: one
/// per color among the sides that have a width and a visible color, or the
/// whole ring when those are every side that has a width. Translucent layers
/// come first, and each layer runs under the opaque ones above it.
std::vector<BorderLayer> borderLayers(const Edges &widths, const facebook::react::BorderColors &colors);

} // namespace rntizen
