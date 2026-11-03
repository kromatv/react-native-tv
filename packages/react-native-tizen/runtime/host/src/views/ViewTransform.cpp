#include "ViewTransform.h"

#include "RenderSync.h"

#include <array>
#include <cmath>

namespace rntizen {

namespace {

using facebook::react::Float;
using facebook::react::Transform;

constexpr std::array<int, 12> kLinearPart{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

bool isTranslationOnly(const Transform &transform) {
  const auto identity = Transform::Identity();
  for (auto index : kLinearPart) {
    if (std::abs(transform.matrix[index] - identity.matrix[index]) > 1e-6f) return false;
  }
  return std::abs(transform.matrix[15] - 1) < 1e-6f;
}

Transform aroundCenterUnlessSet(const facebook::react::ViewProps &props, const facebook::react::LayoutMetrics &metrics) {
  auto matrix = props.resolveTransform(metrics);
  if (props.transformOrigin.isSet()) return matrix;
  const auto &size = metrics.frame.size;
  return Transform::Translate(size.width / 2, size.height / 2, 0) * matrix *
      Transform::Translate(-size.width / 2, -size.height / 2, 0);
}

} // namespace

bool ViewTransform::update(const facebook::react::ViewProps &props, const facebook::react::LayoutMetrics &metrics) {
  auto matrix = props.transform.operations.empty() ? Transform::Identity() : aroundCenterUnlessSet(props, metrics);
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
  const auto localX = static_cast<Float>(point.x - box.x) / scale;
  const auto localY = static_cast<Float>(point.y - box.y) / scale;
  const auto mapped = matrix_ * facebook::react::Vector{.x = localX, .y = localY, .z = 0, .w = 1};
  const auto w = std::abs(mapped.w) > 1e-6f ? mapped.w : 1;
  return {
      .x = box.x + static_cast<Evas_Coord>(std::lround(mapped.x / w * scale)),
      .y = box.y + static_cast<Evas_Coord>(std::lround(mapped.y / w * scale)),
  };
}

} // namespace rntizen
