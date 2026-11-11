#include "SvgFont.h"

#include "SvgPaint.h"

#include <array>
#include <charconv>
#include <string_view>
#include <utility>

namespace rnwebos::svg {

namespace {

constexpr int kNormalWeight = 400;
constexpr int kBoldWeight = 700;

constexpr std::array<std::pair<std::string_view, int>, 4> kNamedWeights{{
    {"normal", kNormalWeight},
    {"bold", kBoldWeight},
    {"bolder", kBoldWeight},
    {"lighter", 300},
}};

std::optional<float> numberFrom(std::string_view text) {
  float value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
  return value;
}

void readSize(const folly::dynamic &value, FontAttributes &attributes) {
  if (value.isString()) {
    const std::string_view text = value.stringPiece();
    if (text.ends_with("em")) {
      attributes.size = numberFrom(text.substr(0, text.size() - 2));
      attributes.relativeSize = attributes.size.has_value();
      return;
    }
  }
  const auto length = lengthFromProp(value);
  if (!length) return;
  attributes.size = length->percent ? length->value / 100 : length->value;
  attributes.relativeSize = length->percent;
}

std::optional<int> weightFrom(const folly::dynamic &value) {
  if (value.isNumber()) return static_cast<int>(value.asDouble());
  if (!value.isString()) return std::nullopt;
  const std::string_view text = value.stringPiece();
  for (const auto &[name, weight] : kNamedWeights) {
    if (text == name) return weight;
  }
  if (const auto number = numberFrom(text)) return static_cast<int>(*number);
  return std::nullopt;
}

std::optional<TextAnchor> anchorFrom(const folly::dynamic &value) {
  if (!value.isString()) return std::nullopt;
  const std::string_view text = value.stringPiece();
  if (text == "middle") return TextAnchor::Middle;
  if (text == "end") return TextAnchor::End;
  if (text == "start") return TextAnchor::Start;
  return std::nullopt;
}

} // namespace

Font FontAttributes::over(const Font &inherited) const {
  Font font = inherited;
  if (family) font.family = *family;
  if (size) font.size = relativeSize ? inherited.size * *size : *size;
  if (weight) font.weight = *weight;
  if (italic) font.italic = *italic;
  if (anchor) font.anchor = *anchor;
  return font;
}

FontAttributes fontAttributesFromProp(const folly::dynamic &font) {
  FontAttributes attributes;
  if (!font.isObject()) return attributes;
  if (const auto *family = font.get_ptr("fontFamily"); family != nullptr && family->isString()) {
    attributes.family = family->getString();
  }
  if (const auto *size = font.get_ptr("fontSize")) readSize(*size, attributes);
  if (const auto *weight = font.get_ptr("fontWeight")) attributes.weight = weightFrom(*weight);
  if (const auto *style = font.get_ptr("fontStyle"); style != nullptr && style->isString()) {
    attributes.italic = style->getString() != "normal";
  }
  if (const auto *anchor = font.get_ptr("textAnchor")) attributes.anchor = anchorFrom(*anchor);
  return attributes;
}

} // namespace rnwebos::svg
