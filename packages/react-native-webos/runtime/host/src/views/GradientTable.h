#pragma once

#include "GradientGeometry.h"
#include "GradientRamp.h"

#include <cstdint>
#include <vector>

namespace rntizen {

/// A gradient's pixels along its line, sampled finely enough that looking one
/// up by the nearest entry differs from the ramp by one step of 255 at most:
/// 16 entries per pixel of the line's steepest change in the patch.
class GradientTable {
 public:
  /// The table for `geometry` and `ramp` painted into a `patchWidth` x
  /// `patchHeight` patch, `scaleX` and `scaleY` patch pixels per box pixel.
  /// Empty when the line does not change or would need too many entries.
  GradientTable(
      const GradientGeometry &geometry,
      const GradientRamp &ramp,
      int patchWidth,
      int patchHeight,
      float scaleX,
      float scaleY);

  bool empty() const;
  /// The pixel nearest `t`, the fraction of the line; ends clamp.
  std::uint32_t at(float t) const;

 private:
  float start_{0};
  float stepsPerUnit_{0};
  std::vector<std::uint32_t> pixels_;
};

} // namespace rntizen
