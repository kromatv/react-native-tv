#include "TextblockFormat.h"

#include "AppFonts.h"
#include "FontMetrics.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace rntizen {

namespace {

using facebook::react::EllipsizeMode;
using facebook::react::Float;
using facebook::react::FontStyle;
using facebook::react::FontWeight;
using facebook::react::TextAlignment;
using facebook::react::TextAttributes;
using facebook::react::TextDecorationLineType;
using facebook::react::TextDecorationStyle;

constexpr Float kDefaultFontSize = 14;
constexpr const char *kDefaultFontFamily = "Tizen";

std::string hexColor(const facebook::react::SharedColor &color, Float opacity, const char *fallback) {
  if (!color) return fallback;
  const auto components = facebook::react::colorComponentsFromColor(color);
  auto channel = [](float value) { return static_cast<int>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255)); };
  char hex[10];
  std::snprintf(
      hex,
      sizeof hex,
      "#%02x%02x%02x%02x",
      channel(components.red),
      channel(components.green),
      channel(components.blue),
      channel(components.alpha * static_cast<float>(opacity)));
  return hex;
}

const char *weightName(std::optional<FontWeight> weight) {
  if (!weight) return "normal";
  const auto value = static_cast<int>(*weight);
  if (value <= 100) return "thin";
  if (value <= 200) return "ultralight";
  if (value <= 300) return "light";
  if (value <= 400) return "normal";
  if (value <= 500) return "medium";
  if (value <= 600) return "semibold";
  if (value <= 700) return "bold";
  if (value <= 800) return "ultrabold";
  return "black";
}

const char *alignmentName(std::optional<TextAlignment> alignment) {
  if (!alignment) return "auto";
  switch (*alignment) {
    case TextAlignment::Center:
      return "center";
    case TextAlignment::Right:
    case TextAlignment::End:
      return "right";
    case TextAlignment::Left:
    case TextAlignment::Start:
    case TextAlignment::Justified:
      return "left";
    case TextAlignment::Natural:
      return "auto";
  }
  return "auto";
}

// A textblock format value ends at the first unescaped space ("One UI Sans").
std::string formatValue(const std::string &value) {
  std::string escapedValue;
  for (char character : value) {
    if (character == ' ' || character == '\\') escapedValue += '\\';
    escapedValue += character;
  }
  return escapedValue;
}

std::string pixels(Float points, Float scale) {
  return std::to_string(std::max(1, static_cast<int>(std::lround(points * scale))));
}

std::string decorationFormat(const TextAttributes &attributes, Float opacity) {
  const auto line = attributes.textDecorationLineType.value_or(TextDecorationLineType::None);
  if (line == TextDecorationLineType::None) return {};
  const auto color = hexColor(
      attributes.textDecorationColor ? attributes.textDecorationColor : attributes.foregroundColor, opacity, "#000000ff");
  std::string format;
  if (line == TextDecorationLineType::Underline || line == TextDecorationLineType::UnderlineStrikethrough) {
    const bool isDouble = attributes.textDecorationStyle == TextDecorationStyle::Double;
    format += isDouble ? " underline=double underline_color=" + color + " underline2_color=" + color
                       : " underline=single underline_color=" + color;
  }
  if (line == TextDecorationLineType::Strikethrough || line == TextDecorationLineType::UnderlineStrikethrough) {
    format += " strikethrough=on strikethrough_color=" + color;
  }
  return format;
}

int pixelCount(Float points, Float scale) {
  return std::max(1, static_cast<int>(std::lround(points * scale)));
}

bool isItalic(const TextAttributes &attributes) {
  return attributes.fontStyle == FontStyle::Italic || attributes.fontStyle == FontStyle::Oblique;
}

const std::string &familyOf(const TextAttributes &attributes) {
  static const std::string fallback = kDefaultFontFamily;
  return attributes.fontFamily.empty() ? fallback : attributes.fontFamily;
}

Float fontSizeOf(const TextAttributes &attributes) {
  return std::isnan(attributes.fontSize) ? kDefaultFontSize : attributes.fontSize;
}

std::string fontName(const TextAttributes &attributes) {
  const auto spacing = attributes.letterSpacing;
  if (!std::isnan(spacing) && spacing != 0) {
    const int weight = attributes.fontWeight ? static_cast<int>(*attributes.fontWeight) : 400;
    if (const auto tracked = trackedAppFont(familyOf(attributes), weight, isItalic(attributes), spacing / fontSizeOf(attributes))) {
      return tracked->string();
    }
  }
  return familyOf(attributes);
}

struct TightLine {
  std::string format;
  int ascent;
  FontMetrics font;
  int lineHeight;
};

std::optional<TightLine> tightLine(const TextAttributes &attributes, Float scale) {
  if (std::isnan(attributes.lineHeight) || attributes.lineHeight <= 0) return std::nullopt;
  const auto fontSize = std::isnan(attributes.fontSize) ? kDefaultFontSize : attributes.fontSize;
  const auto font = fontMetrics(familyOf(attributes), weightName(attributes.fontWeight), isItalic(attributes), pixelCount(fontSize, scale));
  const auto natural = font.ascent + font.descent;
  const auto lineHeight = pixelCount(attributes.lineHeight, scale);
  if (natural <= 0 || lineHeight >= natural) return std::nullopt;
  const auto percent = std::to_string(100.0 * lineHeight / natural);
  const auto ratio = std::strtod(percent.c_str(), nullptr) / 100;
  const auto ascent = static_cast<int>(font.ascent * ratio);
  const auto descent = static_cast<int>(font.descent * ratio);
  return TightLine{
      .format = " linerelsize=" + percent + "% linegap=" + std::to_string(lineHeight - ascent - descent),
      .ascent = ascent,
      .font = font,
      .lineHeight = lineHeight,
  };
}

} // namespace

std::optional<int> tightLineShift(const TextAttributes &attributes, Float scale) {
  const auto tight = tightLine(attributes, scale);
  if (!tight) return std::nullopt;
  const auto &font = tight->font;
  return static_cast<int>(std::lround(font.ascent + (tight->lineHeight - font.ascent - font.descent) / 2.0)) - tight->ascent;
}

std::string fragmentFormat(const TextAttributes &attributes, Float scale) {
  const auto opacity = std::isnan(attributes.opacity) ? Float{1} : attributes.opacity;
  const auto fontSize = fontSizeOf(attributes);
  std::string format = "font=";
  format += formatValue(fontName(attributes));
  format += " font_size=" + pixels(fontSize, scale);
  format += std::string(" font_weight=") + weightName(attributes.fontWeight);
  if (isItalic(attributes)) format += " font_style=italic";
  format += " color=" + hexColor(attributes.foregroundColor, opacity, "#000000ff");
  if (attributes.alignment) format += std::string(" align=") + alignmentName(attributes.alignment);
  if (const auto tight = tightLine(attributes, scale)) {
    format += tight->format;
  } else if (!std::isnan(attributes.lineHeight) && attributes.lineHeight > 0) {
    format += " linesize=" + pixels(attributes.lineHeight, scale);
  }
  format += decorationFormat(attributes, opacity);
  if (attributes.backgroundColor) {
    format += " backing=on backing_color=" + hexColor(attributes.backgroundColor, opacity, "#00000000");
  }
  return format;
}

std::string paragraphFormat(
    const facebook::react::ParagraphAttributes &paragraph,
    std::optional<TextAlignment> alignment,
    Float scale) {
  std::string format = std::string("font=") + kDefaultFontFamily + " font_size=" + pixels(kDefaultFontSize, scale) +
      " color=#000000ff";
  const auto lines = paragraph.maximumNumberOfLines;
  if (lines != 1) format += " wrap=mixed";
  if (lines == 1) {
    switch (paragraph.ellipsizeMode) {
      case EllipsizeMode::Head:
        format += " ellipsis=0.0";
        break;
      case EllipsizeMode::Middle:
        format += " ellipsis=0.5";
        break;
      case EllipsizeMode::Tail:
        format += " ellipsis=1.0";
        break;
      case EllipsizeMode::Clip:
        break;
    }
  }
  format += std::string(" align=") + alignmentName(alignment);
  return format;
}

} // namespace rntizen
