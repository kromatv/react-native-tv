#pragma once

#include "SvgPath.h"

#include <string>

namespace rntizen::svg {

/// An `<Svg>`'s `viewBox` and `preserveAspectRatio`, as react-native-svg's
/// `extractViewBox` splits them. An empty `align` means no viewBox was given.
struct SvgViewBox {
  double minX{0};
  double minY{0};
  double width{0};
  double height{0};
  std::string align;
  int meetOrSlice{0};

  bool present() const;
};

/// The transform from viewBox units to a `width` x `height` view, as
/// react-native-svg computes it on iOS (`RNSVGViewBox getTransform`), after
/// SVG 2's "Computing the equivalent transform of an SVG viewport".
Affine viewBoxTransform(const SvgViewBox &viewBox, double width, double height);

} // namespace rntizen::svg
