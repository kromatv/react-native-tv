#pragma once

#include <react/renderer/graphics/BackgroundImage.h>

namespace rntizen {

/// Where a CSS gradient puts each point of its box on its gradient line, as
/// CSS Images 3 lays out `linear-gradient` and `radial-gradient`, in pixels.
class GradientGeometry {
 public:
  /// The direction an axis-aligned linear gradient varies in, or `None`.
  enum class Axis { None, Vertical, Horizontal };

  GradientGeometry(const facebook::react::BackgroundImage &image, float width, float height, float pointScale);

  /// The line's length in pixels, which a length color stop is measured against.
  float lineLength() const;
  Axis axis() const;
  /// The fraction of the line at the box point (`x`, `y`), in pixels.
  float at(float x, float y) const;
  /// The most `at` changes over one pixel of the box, in any direction.
  float slope() const;
  /// `at(x0 + i * step, y)` for `count` values of `i`, the same fractions to
  /// within a rounding, with the per-pixel division and square root hoisted.
  void row(float x0, float step, float y, int count, float *out) const;

 private:
  void layOutLinear(const facebook::react::LinearGradient &gradient);
  void layOutRadial(const facebook::react::RadialGradient &gradient);

  float width_;
  float height_;
  float pointScale_;
  bool radial_{false};
  float sin_{0};
  float cos_{1};
  float length_{0};
  Axis axis_{Axis::None};
  float centerX_{0};
  float centerY_{0};
  float radiusX_{1};
  float radiusY_{1};
};

} // namespace rntizen
