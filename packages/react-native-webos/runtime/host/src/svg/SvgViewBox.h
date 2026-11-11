#pragma once

#include "include/core/SkMatrix.h"

#include <string>

namespace rnwebos::svg {

/// An `<Svg>`'s `viewBox` and `preserveAspectRatio`, as react-native-svg's
/// `extractViewBox` splits them. An empty `align` means no viewBox was given.
struct SvgViewBox {
  float minX{0};
  float minY{0};
  float width{0};
  float height{0};
  std::string align;
  int meetOrSlice{0};

  bool present() const;
};

/// The transform from viewBox units to a `width` x `height` view, as
/// react-native-svg computes it on iOS (`RNSVGViewBox getTransform`), after
/// SVG 2's "Computing the equivalent transform of an SVG viewport".
SkMatrix viewBoxTransform(const SvgViewBox &viewBox, float width, float height);

} // namespace rnwebos::svg
