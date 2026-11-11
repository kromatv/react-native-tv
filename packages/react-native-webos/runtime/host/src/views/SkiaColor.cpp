#include "SkiaColor.h"

namespace rnwebos {

SkColor4f skColor(const facebook::react::SharedColor &color, float opacity) {
  if (!color) return SkColors::kTransparent;
  const auto components = facebook::react::colorComponentsFromColor(color);
  return {components.red, components.green, components.blue, components.alpha * opacity};
}

bool isVisible(const facebook::react::SharedColor &color) {
  return color && facebook::react::colorComponentsFromColor(color).alpha > 0;
}

bool isOpaque(const facebook::react::SharedColor &color) {
  return color && facebook::react::colorComponentsFromColor(color).alpha >= 1;
}

} // namespace rnwebos
