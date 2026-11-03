#include "ParagraphView.h"

#include "../mounting/ComponentViewRegistry.h"
#include "../text/LineClamp.h"
#include "../text/LineSpan.h"
#include "../text/TextblockMarkup.h"

#include <cmath>

namespace rntizen {

ParagraphView::ParagraphView(Evas *evas, facebook::react::Tag tag)
    : ComponentView(evas, tag), textblock_(evas_object_textblock_add(evas)), style_(evas_textblock_style_new()) {
  evas_object_textblock_style_set(textblock_, style_);
  evas_object_pass_events_set(textblock_, EINA_TRUE);
  addMember(textblock_, true);
}

ParagraphView::~ParagraphView() {
  evas_object_del(textblock_);
  evas_textblock_style_free(style_);
}

void ParagraphView::updateState(const facebook::react::State::Shared &state) {
  if (!state) return;
  paragraph_ = std::static_pointer_cast<const facebook::react::ParagraphShadowNode::ConcreteState>(state);
  applyContent();
}

void ParagraphView::updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) {
  const bool rescaled = layoutMetrics.pointScaleFactor != contentScale_;
  ComponentView::updateLayout(layoutMetrics);
  if (rescaled) applyContent();
}

void ParagraphView::applyContent() {
  if (!paragraph_) return;
  contentScale_ = layoutMetrics().pointScaleFactor;
  const auto &paragraph = paragraph_->getData();
  const auto content = textblockContent(paragraph.attributedString, paragraph.paragraphAttributes, pointScale());
  markup_ = content.markup;
  tightLineShift_ = content.tightLineShift;
  firstLineTop_ = -1;
  evas_textblock_style_set(style_, content.style.c_str());
  evas_object_textblock_text_markup_set(textblock_, markup_.c_str());
  clampedWidth_ = -1;
  clamped_ = false;
  layoutMembers();
}

void ParagraphView::layoutMembers() {
  const auto &box = canvasRect();
  const auto scale = pointScale();
  const auto content = layoutMetrics().getContentFrame();
  const auto width = static_cast<Evas_Coord>(std::ceil(content.size.width * scale));
  const auto x = box.x + toPixels(content.origin.x, scale);
  const auto y = box.y + toPixels(content.origin.y, scale);
  const auto height = static_cast<Evas_Coord>(std::ceil(content.size.height * scale));
  if (tightLineShift_ && firstLineTop_ < 0) {
    evas_object_geometry_set(textblock_, x, y, width, height);
    firstLineTop_ = lineSpan(textblock_, 1).top;
  }
  const auto top = tightLineShift_ ? firstLineTop_ : 0;
  evas_object_geometry_set(textblock_, x, y - top + tightLineShift_.value_or(0), width, height + top);
  if (!paragraph_ || width <= 0 || width == clampedWidth_) return;
  const auto &attributes = paragraph_->getData().paragraphAttributes;
  if (attributes.maximumNumberOfLines <= 1) return;
  if (clamped_) evas_object_textblock_text_markup_set(textblock_, markup_.c_str());
  clampLines(textblock_, attributes.maximumNumberOfLines, attributes.ellipsizeMode != facebook::react::EllipsizeMode::Clip);
  clampedWidth_ = width;
  clamped_ = true;
}

RN_TIZEN_COMPONENT_VIEW("Paragraph", ParagraphView);

} // namespace rntizen
