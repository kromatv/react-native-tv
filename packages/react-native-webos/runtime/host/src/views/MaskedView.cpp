#include "MaskedView.h"

#include "../mounting/ComponentViewRegistry.h"

#include "include/core/SkPaint.h"

namespace rnwebos {

// The children after the mask are drawn into a layer, and the mask into a
// second one that keeps of the first what its alpha covers (destination-in).
void MaskedView::drawChildren(SkCanvas &canvas) {
  const auto all = children();
  if (all.size() < 2) return;
  const auto size = pixelSize();
  const auto box = SkRect::MakeWH(static_cast<float>(size.width), static_cast<float>(size.height));
  SkAutoCanvasRestore restore(&canvas, true);
  canvas.clipRect(box);
  canvas.saveLayer(&box, nullptr);
  for (auto *child : all.subspan(1)) drawChild(canvas, *child);
  SkPaint keepCovered;
  keepCovered.setBlendMode(SkBlendMode::kDstIn);
  canvas.saveLayer(&box, &keepCovered);
  drawChild(canvas, *all.front());
}

RN_WEBOS_COMPONENT_VIEW("RNCMaskedView", MaskedView);

} // namespace rnwebos
