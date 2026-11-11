#include "ViewTransform.h"

#include "TransformMath.h"

#include <cmath>

namespace rnwebos {

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
  isIdentity_ = matrix_ == Transform::Identity();
  isTranslation_ = isTranslationOnly(matrix_);
  return true;
}

bool ViewTransform::isIdentity() const {
  return isIdentity_;
}

SkM44 ViewTransform::pixelMatrix(Float scale) const {
  const auto s = static_cast<float>(scale);
  return SkM44::Scale(s, s, s) * SkM44::ColMajor(matrix_.matrix.data()) * SkM44::Scale(1 / s, 1 / s, 1 / s);
}

void ViewTransform::concat(SkCanvas &canvas, Float scale) const {
  if (isIdentity_ || scale <= 0) return;
  if (isTranslation_) {
    canvas.translate(std::round(matrix_.matrix[12] * scale), std::round(matrix_.matrix[13] * scale));
    return;
  }
  canvas.concat(pixelMatrix(scale));
}

SkPoint ViewTransform::mapPoint(SkPoint local, Float scale) const {
  if (isIdentity_ || scale <= 0) return local;
  if (isTranslation_) {
    return {local.x() + std::round(matrix_.matrix[12] * scale), local.y() + std::round(matrix_.matrix[13] * scale)};
  }
  const auto mapped = pixelMatrix(scale).map(local.x(), local.y(), 0, 1);
  const auto w = std::abs(mapped.w) > 1e-6f ? mapped.w : 1;
  return {mapped.x / w, mapped.y / w};
}

bool ViewTransform::invertPoint(SkPoint outer, Float scale, SkPoint &local) const {
  if (isIdentity_ || scale <= 0) {
    local = outer;
    return true;
  }
  SkMatrix inverse;
  if (!pixelMatrix(scale).asM33().invert(&inverse)) return false;
  local = inverse.mapPoint(outer);
  return true;
}

} // namespace rnwebos
