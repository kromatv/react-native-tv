#pragma once

#include <react/renderer/graphics/Float.h>

#include "include/core/SkMatrix.h"

#include <vector>

namespace rnwebos::svg {

/// A react-native-svg `matrix` or `gradientTransform` prop: SVG's
/// `matrix(a, b, c, d, e, f)` as six numbers, the identity for any other count.
SkMatrix matrixFromProp(const std::vector<facebook::react::Float> &matrix);

} // namespace rnwebos::svg
