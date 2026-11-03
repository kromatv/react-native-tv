#include "EvasTextMeasurer.h"

#include "../threading/MainThread.h"
#include "LineSpan.h"
#include "TextblockMarkup.h"

#include <algorithm>
#include <cmath>

namespace rntizen {

namespace {

constexpr int kUnboundedWidth = 100000;

int widthLimit(facebook::react::Float width) {
  if (std::isinf(width) || std::isnan(width) || width > kUnboundedWidth) return kUnboundedWidth;
  return std::max(1, static_cast<int>(std::floor(width)));
}

} // namespace

EvasTextMeasurer::EvasTextMeasurer()
    : canvas_(ecore_evas_buffer_new(1, 1)),
      textblock_(evas_object_textblock_add(ecore_evas_get(canvas_))),
      style_(evas_textblock_style_new()) {
  evas_object_textblock_style_set(textblock_, style_);
}

EvasTextMeasurer::~EvasTextMeasurer() {
  evas_object_del(textblock_);
  evas_textblock_style_free(style_);
  ecore_evas_free(canvas_);
}

facebook::react::Size EvasTextMeasurer::measure(
    const facebook::react::AttributedString &attributedString,
    const facebook::react::ParagraphAttributes &paragraphAttributes,
    facebook::react::Size maximumSize,
    facebook::react::Float pointScaleFactor) const {
  const auto scale = pointScaleFactor > 0 ? pointScaleFactor : 1;
  const auto widthPixels = widthLimit(maximumSize.width * scale);
  const UnwrappedSizes::Key key{
      .attributedString = attributedString,
      .paragraphAttributes = paragraphAttributes,
      .layoutConstraints = {},
      .pointScaleFactor = scale,
  };
  if (const auto size = unwrapped_.find(key, widthPixels)) return *size;
  facebook::react::Size measured{};
  std::optional<Evas_Coord> unwrappedWidth;
  runOnMainThreadSoon([&] {
    const auto content = textblockContent(attributedString, paragraphAttributes, scale);
    evas_textblock_style_set(style_, content.style.c_str());
    evas_object_textblock_text_markup_set(textblock_, content.markup.c_str());
    evas_object_resize(textblock_, widthPixels, 0);
    Evas_Coord width = 0;
    Evas_Coord height = 0;
    evas_object_textblock_size_formatted_get(textblock_, &width, &height);
    Evas_Coord nativeWidth = 0;
    Evas_Coord nativeHeight = 0;
    evas_object_textblock_size_native_get(textblock_, &nativeWidth, &nativeHeight);
    if (width == nativeWidth && height == nativeHeight) unwrappedWidth = width;
    const auto lines = paragraphAttributes.maximumNumberOfLines;
    Evas_Coord lineY = 0;
    Evas_Coord lineHeight = 0;
    if (lines > 0 && evas_object_textblock_line_number_geometry_get(textblock_, lines, nullptr, nullptr, nullptr, nullptr) &&
        evas_object_textblock_line_number_geometry_get(textblock_, lines - 1, nullptr, &lineY, nullptr, &lineHeight)) {
      height = lineY + lineHeight;
    }
    if (content.tightLineShift) {
      const auto span = lineSpan(textblock_, lines);
      height = span.bottom - span.top;
    }
    measured = {
        .width = static_cast<facebook::react::Float>(width) / scale,
        .height = static_cast<facebook::react::Float>(height) / scale,
    };
  });
  if (unwrappedWidth) unwrapped_.remember(key, *unwrappedWidth, measured);
  return measured;
}

} // namespace rntizen
