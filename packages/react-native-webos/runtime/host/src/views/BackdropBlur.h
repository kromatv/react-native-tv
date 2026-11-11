#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkRRect.h"

namespace rnwebos {

/// What is drawn behind a box, blurred: Skia filters the pixels the frame has
/// under the box into a layer and draws it back, cut to the box
/// (docs/reference/skia-rendering.md, "What the views draw"). It costs a
/// layer and a blur in every frame that draws it.
class BackdropBlur final {
 public:
  /// The blur's Gaussian deviation in pixels; 0 blurs nothing.
  void setSigma(float pixels);
  float sigma() const;

  /// Blurs what `canvas` holds under `box` and blends it back at `opacity`.
  void draw(SkCanvas &canvas, const SkRRect &box, float opacity) const;

 private:
  sk_sp<SkImageFilter> filter_;
  float sigma_{0};
};

} // namespace rnwebos
