#include "EvasColor.h"

#include <algorithm>
#include <cmath>

namespace rntizen {

namespace {

int toByte(float unit) {
  return static_cast<int>(std::lround(std::clamp(unit, 0.0f, 1.0f) * 255));
}

} // namespace

void setColor(Evas_Object *object, const facebook::react::SharedColor &color, float opacity) {
  if (!color) {
    evas_object_color_set(object, 0, 0, 0, 0);
    return;
  }
  const auto components = facebook::react::colorComponentsFromColor(color);
  const auto alpha = std::clamp(components.alpha * opacity, 0.0f, 1.0f);
  evas_object_color_set(
      object, toByte(components.red * alpha), toByte(components.green * alpha), toByte(components.blue * alpha), toByte(alpha));
}

bool isVisible(const facebook::react::SharedColor &color) {
  return color && facebook::react::colorComponentsFromColor(color).alpha > 0;
}

} // namespace rntizen
