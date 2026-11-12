#include "TransformMath.h"

#include <array>
#include <cmath>

namespace rnwebos {

namespace {

using facebook::react::Float;
using facebook::react::Transform;

constexpr std::array<int, 12> kLinearPart{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
constexpr Float kEpsilon = 1e-6f;

} // namespace

bool isTranslationOnly(const Transform &transform) {
  const auto identity = Transform::Identity();
  for (auto index : kLinearPart) {
    if (std::abs(transform.matrix[index] - identity.matrix[index]) > kEpsilon) return false;
  }
  return std::abs(transform.matrix[15] - 1) < kEpsilon;
}

Transform aroundCenter(const Transform &transform, const facebook::react::Size &size) {
  return Transform::Translate(size.width / 2, size.height / 2, 0) * transform *
      Transform::Translate(-size.width / 2, -size.height / 2, 0);
}

} // namespace rnwebos
