#pragma once

#include "NinePatch.h"

#include <react/renderer/graphics/BackgroundImage.h>

namespace rntizen {

/// The box a gradient layer paints, in pixels, with the corners it is cut to.
struct GradientBox {
  int width{0};
  int height{0};
  float pointScale{1};
  Corners corners;
};

/// `image` painted over `box`, its rounded corners included, as a patch the
/// image object stretches to the box. An axis-aligned linear gradient is one
/// pixel per step along its line, the corners kept as nine-patch borders
/// across it; any other gradient is the whole box, scaled down when it would
/// pass 512x512 pixels. No side passes 2048 pixels, below the texture size
/// the GL engine draws (efl-rendering.md, section 4). An empty patch paints
/// nothing. Patches are cached; a returned reference stays valid until the
/// next call. Main thread only.
const NinePatch &gradientPatch(const facebook::react::BackgroundImage &image, const GradientBox &box);

} // namespace rntizen
