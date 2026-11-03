#include "ViewTransform.h"

#include "RenderSync.h"
#include "TransformMath.h"

#include <array>

namespace rntizen {

using facebook::react::Float;
using facebook::react::Transform;

bool ViewTransform::update(const facebook::react::ViewProps &props, const facebook::react::LayoutMetrics &metrics) {
  auto matrix = Transform::Identity();
  if (!props.transform.operations.empty()) {
    matrix = props.resolveTransform(metrics);
    if (!props.transformOrigin.isSet()) matrix = aroundCenter(matrix, metrics.frame.size);
  }
  if (matrix == matrix_) return false;
  matrix_ = matrix;
  isTranslation_ = isTranslationOnly(matrix_);
  return true;
}

PixelPoint ViewTransform::translation(Float scale) const {
  if (!isTranslation_) return {};
  return {.x = toPixels(matrix_.matrix[12], scale), .y = toPixels(matrix_.matrix[13], scale)};
}

void ViewTransform::apply(Evas_Object *root, const PixelRect &box, const PixelRect &bounds, Float scale) const {
  if (isTranslation_ || scale <= 0) {
    if (evas_object_map_enable_get(root)) {
      waitForRenderThread(root);
      evas_object_map_enable_set(root, EINA_FALSE);
    }
    return;
  }
  const std::array<std::array<Evas_Coord, 2>, 4> corners{{
      {bounds.x, bounds.y},
      {bounds.x + bounds.width, bounds.y},
      {bounds.x + bounds.width, bounds.y + bounds.height},
      {bounds.x, bounds.y + bounds.height},
  }};
  auto *map = evas_map_new(4);
  for (int i = 0; i < 4; ++i) {
    const auto mapped = mapPoint({.x = corners[i][0], .y = corners[i][1]}, box, scale);
    evas_map_point_coord_set(map, i, mapped.x, mapped.y, 0);
    evas_map_point_image_uv_set(map, i, corners[i][0] - bounds.x, corners[i][1] - bounds.y);
    evas_map_point_color_set(map, i, 255, 255, 255, 255);
  }
  evas_map_smooth_set(map, EINA_TRUE);
  evas_map_alpha_set(map, EINA_TRUE);
  evas_object_map_set(root, map);
  evas_object_map_enable_set(root, EINA_TRUE);
  evas_map_free(map);
}

PixelPoint ViewTransform::mapPoint(PixelPoint point, const PixelRect &box, Float scale) const {
  if (isTranslation_ || scale <= 0) return point;
  return transformedPoint(matrix_, point, box, scale);
}

} // namespace rntizen
