#include "BackdropBlur.h"

#include "include/core/SkPaint.h"
#include "include/effects/SkImageFilters.h"

namespace rnwebos {

void BackdropBlur::setSigma(float pixels) {
  if (pixels == sigma_) return;
  sigma_ = pixels;
  // Clamped, so the blur meets the screen's edges with the pixels at them
  // rather than fading out.
  filter_ = pixels > 0 ? SkImageFilters::Blur(pixels, pixels, SkTileMode::kClamp, nullptr) : nullptr;
}

float BackdropBlur::sigma() const {
  return sigma_;
}

void BackdropBlur::draw(SkCanvas &canvas, const SkRRect &box, float opacity) const {
  if (!filter_ || box.isEmpty() || opacity <= 0) return;
  SkAutoCanvasRestore restore(&canvas, true);
  if (box.isRect()) {
    canvas.clipRect(box.rect(), false);
  } else {
    canvas.clipRRect(box, true);
  }
  SkPaint blend;
  blend.setAlphaf(opacity);
  canvas.saveLayer(SkCanvas::SaveLayerRec(&box.rect(), opacity < 1 ? &blend : nullptr, filter_.get(), 0));
}

} // namespace rnwebos
