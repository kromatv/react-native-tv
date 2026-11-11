#include "ComponentView.h"

#include "../mounting/ComponentViewRegistry.h"

#include "include/core/SkPaint.h"

namespace rnwebos {

namespace {

// The first child is the mask: the others are drawn into a layer, and the
// mask into a second one that keeps of the first what its alpha covers.
class MaskedView final : public ComponentView {
 public:
  using ComponentView::ComponentView;

 protected:
  void drawChildren(SkCanvas &canvas) override {
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
};

} // namespace

RN_WEBOS_COMPONENT_VIEW("RNCMaskedView", MaskedView);

} // namespace rnwebos
