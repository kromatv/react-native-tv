#include "TextStyles.h"

#include "../views/SkiaColor.h"

#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"

#include <array>
#include <cmath>
#include <utility>

namespace rnwebos {

namespace {

using facebook::react::Float;
using facebook::react::FontVariant;
using facebook::react::TextAttributes;
using facebook::react::TextDecorationLineType;
using namespace skia::textlayout;

constexpr Float kDefaultFontSize = 14;

constexpr std::array<std::pair<FontVariant, const char *>, 5> kFontFeatures{{
    {FontVariant::SmallCaps, "smcp"},
    {FontVariant::OldstyleNums, "onum"},
    {FontVariant::LiningNums, "lnum"},
    {FontVariant::TabularNums, "tnum"},
    {FontVariant::ProportionalNums, "pnum"},
}};

SkFontStyle fontStyle(const TextAttributes &attributes) {
  const int weight = attributes.fontWeight ? static_cast<int>(*attributes.fontWeight) : SkFontStyle::kNormal_Weight;
  const bool italic = attributes.fontStyle == facebook::react::FontStyle::Italic ||
      attributes.fontStyle == facebook::react::FontStyle::Oblique;
  return SkFontStyle(weight, SkFontStyle::kNormal_Width, italic ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant);
}

TextDecorationStyle decorationStyle(std::optional<facebook::react::TextDecorationStyle> style) {
  using enum facebook::react::TextDecorationStyle;
  switch (style.value_or(Solid)) {
    case Double:
      return TextDecorationStyle::kDouble;
    case Dotted:
      return TextDecorationStyle::kDotted;
    case Dashed:
      return TextDecorationStyle::kDashed;
    case Wavy:
      return TextDecorationStyle::kWavy;
    case Solid:
      return TextDecorationStyle::kSolid;
  }
  return TextDecorationStyle::kSolid;
}

void applyDecoration(TextStyle &style, const TextAttributes &attributes, float opacity) {
  using enum TextDecorationLineType;
  const auto line = attributes.textDecorationLineType.value_or(None);
  if (line == None) return;
  int decoration = TextDecoration::kNoDecoration;
  if (line == Underline || line == UnderlineStrikethrough) decoration |= TextDecoration::kUnderline;
  if (line == Strikethrough || line == UnderlineStrikethrough) decoration |= TextDecoration::kLineThrough;
  style.setDecoration(static_cast<TextDecoration>(decoration));
  style.setDecorationStyle(decorationStyle(attributes.textDecorationStyle));
  const auto &color = attributes.textDecorationColor ? attributes.textDecorationColor : attributes.foregroundColor;
  style.setDecorationColor(color ? skColor(color, opacity).toSkColor() : SK_ColorBLACK);
}

} // namespace

TextStyle textStyle(const TextAttributes &attributes, Float scale) {
  const auto opacity = std::isnan(attributes.opacity) ? 1.0f : static_cast<float>(attributes.opacity);
  const auto fontSize = (std::isnan(attributes.fontSize) ? kDefaultFontSize : attributes.fontSize) * scale;
  TextStyle style;
  SkPaint foreground;
  foreground.setAntiAlias(true);
  foreground.setColor4f(attributes.foregroundColor ? skColor(attributes.foregroundColor, opacity) : SkColors::kBlack);
  style.setForegroundPaint(foreground);
  if (isVisible(attributes.backgroundColor)) {
    SkPaint background;
    background.setColor4f(skColor(attributes.backgroundColor, opacity));
    style.setBackgroundPaint(background);
  }
  if (!attributes.fontFamily.empty()) style.setFontFamilies({SkString(attributes.fontFamily.c_str())});
  style.setFontSize(static_cast<float>(fontSize));
  style.setFontStyle(fontStyle(attributes));
  if (!std::isnan(attributes.letterSpacing)) style.setLetterSpacing(static_cast<float>(attributes.letterSpacing * scale));
  if (!std::isnan(attributes.lineHeight) && attributes.lineHeight > 0 && fontSize > 0) {
    style.setHeight(static_cast<float>(attributes.lineHeight * scale / fontSize));
    style.setHeightOverride(true);
    style.setHalfLeading(true);
  }
  applyDecoration(style, attributes, opacity);
  if (isVisible(attributes.textShadowColor)) {
    const auto offset = attributes.textShadowOffset.value_or(facebook::react::Size{});
    const auto radius = std::isnan(attributes.textShadowRadius) ? Float{0} : attributes.textShadowRadius;
    style.addShadow(TextShadow(
        skColor(attributes.textShadowColor, opacity).toSkColor(),
        {static_cast<float>(offset.width * scale), static_cast<float>(offset.height * scale)},
        radius * scale / 2));
  }
  if (attributes.fontVariant) {
    const auto variants = static_cast<int>(*attributes.fontVariant);
    for (const auto &[variant, feature] : kFontFeatures) {
      if ((variants & static_cast<int>(variant)) != 0) style.addFontFeature(SkString(feature), 1);
    }
  }
  return style;
}

} // namespace rnwebos
