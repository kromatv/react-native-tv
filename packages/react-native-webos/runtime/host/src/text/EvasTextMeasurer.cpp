#include "EvasTextMeasurer.h"

#include "../threading/MainThread.h"
#include "LineSpan.h"
#include "TextblockMarkup.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace rntizen {

namespace {

constexpr int kUnboundedWidth = 100000;

struct Measurement {
  facebook::react::Size size;
  std::optional<Evas_Coord> unwrappedWidth;
};

int widthLimit(facebook::react::Float width) {
  if (std::isinf(width) || std::isnan(width) || width > kUnboundedWidth) return kUnboundedWidth;
  return std::max(1, static_cast<int>(std::floor(width)));
}

Measurement layOut(
    Evas_Object *textblock,
    Evas_Textblock_Style *style,
    const TextblockContent &content,
    int lines,
    int widthPixels,
    facebook::react::Float scale) {
  evas_textblock_style_set(style, content.style.c_str());
  evas_object_textblock_text_markup_set(textblock, content.markup.c_str());
  evas_object_resize(textblock, widthPixels, 0);
  Evas_Coord width = 0;
  Evas_Coord height = 0;
  evas_object_textblock_size_formatted_get(textblock, &width, &height);
  Evas_Coord nativeWidth = 0;
  Evas_Coord nativeHeight = 0;
  evas_object_textblock_size_native_get(textblock, &nativeWidth, &nativeHeight);
  Measurement measurement{};
  if (width == nativeWidth && height == nativeHeight) measurement.unwrappedWidth = width;
  Evas_Coord lineY = 0;
  Evas_Coord lineHeight = 0;
  if (lines > 0 && evas_object_textblock_line_number_geometry_get(textblock, lines, nullptr, nullptr, nullptr, nullptr) &&
      evas_object_textblock_line_number_geometry_get(textblock, lines - 1, nullptr, &lineY, nullptr, &lineHeight)) {
    height = lineY + lineHeight;
  }
  if (content.tightLineShift.has_value()) {
    const auto span = lineSpan(textblock, lines);
    height = span.bottom - span.top;
  }
  measurement.size = {
      .width = static_cast<facebook::react::Float>(width) / scale,
      .height = static_cast<facebook::react::Float>(height) / scale,
  };
  return measurement;
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
  Measurement measured{};
  runOnMainThreadSoon([&] {
    const auto content = textblockContent(attributedString, paragraphAttributes, scale);
    measured = layOut(textblock_, style_, content, paragraphAttributes.maximumNumberOfLines, widthPixels, scale);
  });
  if (measured.unwrappedWidth.has_value()) unwrapped_.remember(key, *measured.unwrappedWidth, measured.size);
  return measured.size;
}

} // namespace rntizen
