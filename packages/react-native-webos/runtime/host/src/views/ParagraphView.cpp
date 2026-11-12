#include "ParagraphView.h"

#include "../mounting/ComponentViewRegistry.h"

#include <cmath>

namespace rnwebos {

void ParagraphView::updateState(const facebook::react::State::Shared &state) {
  if (!state) return;
  paragraph_ = std::static_pointer_cast<const facebook::react::ParagraphShadowNode::ConcreteState>(state);
  prepared_ = std::dynamic_pointer_cast<const PreparedParagraph>(
      paragraph_->getData().measuredLayout.preparedTextLayout);
  layout_ = {};
  invalidate();
}

void ParagraphView::layoutContent() {
  if (contentRect() != laidOutIn_ || pointScale() != laidOutScale_) layout_ = {};
}

PixelRect ParagraphView::contentRect() const {
  const auto scale = pointScale();
  const auto content = layoutMetrics().getContentFrame();
  return {
      .x = toPixels(content.origin.x, scale),
      .y = toPixels(content.origin.y, scale),
      .width = static_cast<int>(std::ceil(content.size.width * scale)),
      .height = static_cast<int>(std::ceil(content.size.height * scale)),
  };
}

int ParagraphView::contentLayers() const {
  // A paragraph's runs carry their own colors, so its opacity needs a layer.
  return 2;
}

void ParagraphView::drawContent(SkCanvas &canvas, float) {
  if (!paragraph_) return;
  const auto rect = contentRect();
  if (rect.width <= 0) return;
  if (prepared_ && prepared_->scale == pointScale()) {
    SkAutoCanvasRestore restore(&canvas, true);
    canvas.translate(static_cast<float>(rect.x), static_cast<float>(rect.y));
    prepared_->picture->playback(&canvas);
    return;
  }
  if (!layout_.paragraph) {
    const auto scale = pointScale();
    const auto &data = paragraph_->getData();
    layout_ = layOutParagraph(data.attributedString, data.paragraphAttributes, static_cast<float>(rect.width) / scale, scale);
    laidOutIn_ = rect;
    laidOutScale_ = scale;
  }
  layout_.paragraph->paint(&canvas, static_cast<float>(rect.x), static_cast<float>(rect.y));
}

RN_WEBOS_COMPONENT_VIEW("Paragraph", ParagraphView);

} // namespace rnwebos
