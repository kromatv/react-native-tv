#include "BoxPainter.h"

#include "GradientShader.h"

#include "include/core/SkPaint.h"

namespace rnwebos {

void drawBackgroundImages(
    SkCanvas &canvas,
    const SkRRect &box,
    const std::vector<facebook::react::BackgroundImage> &images,
    float pointScale,
    float alpha) {
  const auto &rect = box.rect();
  if (images.empty() || rect.isEmpty()) return;
  const bool moved = rect.left() != 0 || rect.top() != 0;
  SkAutoCanvasRestore restore(&canvas, moved);
  if (moved) canvas.translate(rect.left(), rect.top());
  const auto shape = box.makeOffset(-rect.left(), -rect.top());
  SkPaint paint;
  paint.setAntiAlias(true);
  // An 8-bit gradient over a long box shows bands on a TV; Skia spreads the
  // rounding error when asked.
  paint.setDither(true);
  paint.setAlphaf(alpha);
  for (auto image = images.rbegin(); image != images.rend(); ++image) {
    const auto &shader = gradientShader(*image, rect.width(), rect.height(), pointScale);
    if (!shader) continue;
    paint.setShader(shader);
    if (shape.isRect()) {
      canvas.drawRect(shape.rect(), paint);
    } else {
      canvas.drawRRect(shape, paint);
    }
  }
}

} // namespace rnwebos
