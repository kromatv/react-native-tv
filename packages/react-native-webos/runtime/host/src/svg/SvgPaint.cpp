#include "SvgPaint.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <string_view>
#include <utility>

namespace rnwebos::svg {

namespace {

constexpr int kSolidColorBrush = 0;
constexpr int kReferenceBrush = 1;
constexpr int kCurrentColorBrush = 2;

constexpr std::array<std::pair<std::string_view, PaintAttribute>, 11> kPropNames{{
    {"fill", PaintAttribute::Fill},
    {"fillOpacity", PaintAttribute::FillOpacity},
    {"fillRule", PaintAttribute::FillRule},
    {"stroke", PaintAttribute::Stroke},
    {"strokeOpacity", PaintAttribute::StrokeOpacity},
    {"strokeWidth", PaintAttribute::StrokeWidth},
    {"strokeLinecap", PaintAttribute::StrokeLinecap},
    {"strokeLinejoin", PaintAttribute::StrokeLinejoin},
    {"strokeMiterlimit", PaintAttribute::StrokeMiterlimit},
    {"strokeDasharray", PaintAttribute::StrokeDasharray},
    {"strokeDashoffset", PaintAttribute::StrokeDashoffset},
}};

constexpr std::array<std::pair<std::string_view, float>, 7> kUnitsInUserUnits{{
    {"", 1},
    {"px", 1},
    {"pt", 1.25},
    {"pc", 15},
    {"mm", 3.543307F},
    {"cm", 35.43307F},
    {"in", 90},
}};

void copyAttribute(Paint &to, const Paint &from, PaintAttribute attribute) {
  using enum PaintAttribute;
  switch (attribute) {
    case Fill:
      to.fill = from.fill;
      return;
    case FillOpacity:
      to.fillOpacity = from.fillOpacity;
      return;
    case FillRule:
      to.fillRule = from.fillRule;
      return;
    case Stroke:
      to.stroke = from.stroke;
      return;
    case StrokeOpacity:
      to.strokeOpacity = from.strokeOpacity;
      return;
    case StrokeWidth:
      to.strokeWidth = from.strokeWidth;
      return;
    case StrokeLinecap:
      to.strokeLinecap = from.strokeLinecap;
      return;
    case StrokeLinejoin:
      to.strokeLinejoin = from.strokeLinejoin;
      return;
    case StrokeMiterlimit:
      to.strokeMiterlimit = from.strokeMiterlimit;
      return;
    case StrokeDasharray:
      to.strokeDasharray = from.strokeDasharray;
      return;
    case StrokeDashoffset:
      to.strokeDashoffset = from.strokeDashoffset;
      return;
    case Count:
      return;
  }
}

std::optional<SkColor4f> colorFromNumber(const folly::dynamic &value) {
  if (!value.isNumber()) return std::nullopt;
  return colorFromArgb(static_cast<std::uint32_t>(static_cast<std::int64_t>(value.asDouble())));
}

std::string_view trimmed(std::string_view text) {
  const auto first = text.find_first_not_of(" \t\n\r");
  if (first == std::string_view::npos) return {};
  const auto last = text.find_last_not_of(" \t\n\r");
  return text.substr(first, last - first + 1);
}

std::optional<Length> lengthFromString(std::string_view text) {
  text = trimmed(text);
  if (!text.empty() && text.front() == '+') text.remove_prefix(1);
  float value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{}) return std::nullopt;
  const auto unit = trimmed(text.substr(static_cast<std::size_t>(end - text.data())));
  if (unit == "%") return Length{.value = value, .percent = true};
  for (const auto &[name, scale] : kUnitsInUserUnits) {
    if (unit == name) return Length{.value = value * scale};
  }
  return std::nullopt;
}

} // namespace

float Length::resolve(float reference) const {
  return percent ? value * reference / 100 : value;
}

float Length::fractionOf(float reference) const {
  return (percent ? value / 100 : value) * reference;
}

Paint inheritPaint(const Paint &own, const Paint &parent) {
  Paint resolved = own;
  for (std::size_t index = 0; index < resolved.listed.size(); ++index) {
    if (own.listed[index] || !parent.listed[index]) continue;
    copyAttribute(resolved, parent, static_cast<PaintAttribute>(index));
    resolved.listed.set(index);
  }
  return resolved;
}

PaintAttributes paintAttributesFromPropList(const std::vector<std::string> &propList) {
  PaintAttributes attributes;
  for (const auto &prop : propList) {
    for (const auto &[name, attribute] : kPropNames) {
      if (prop == name) attributes.set(static_cast<std::size_t>(attribute));
    }
  }
  return attributes;
}

Brush brushFromProp(const folly::dynamic &value) {
  using enum Brush::Kind;
  if (const auto color = colorFromNumber(value)) return {.kind = Color, .color = *color};
  if (!value.isObject()) return {};
  const auto *type = value.get_ptr("type");
  if (type == nullptr || !type->isNumber()) return {};
  const auto kind = static_cast<int>(type->asDouble());
  if (kind == kCurrentColorBrush) return {.kind = CurrentColor};
  if (kind == kReferenceBrush) {
    const auto *reference = value.get_ptr("brushRef");
    if (reference == nullptr || !reference->isString()) return {};
    return {.kind = Reference, .reference = reference->getString()};
  }
  if (kind != kSolidColorBrush) return {};
  const auto *payload = value.get_ptr("payload");
  const auto color = payload == nullptr ? std::nullopt : colorFromNumber(*payload);
  if (!color) return {};
  return {.kind = Color, .color = *color};
}

std::optional<Length> lengthFromProp(const folly::dynamic &value) {
  if (value.isNumber()) return Length{.value = static_cast<float>(value.asDouble())};
  if (value.isString()) return lengthFromString(value.getString());
  return std::nullopt;
}

std::vector<Length> lengthsFromProp(const folly::dynamic &value) {
  std::vector<Length> lengths;
  if (!value.isArray()) {
    if (const auto single = lengthFromProp(value)) lengths.push_back(*single);
    return lengths;
  }
  lengths.reserve(value.size());
  for (const auto &entry : value) {
    if (const auto length = lengthFromProp(entry)) lengths.push_back(*length);
  }
  return lengths;
}

SkColor4f colorFromArgb(std::uint32_t argb) {
  return {
      static_cast<float>((argb >> 16) & 0xff) / 255,
      static_cast<float>((argb >> 8) & 0xff) / 255,
      static_cast<float>(argb & 0xff) / 255,
      static_cast<float>((argb >> 24) & 0xff) / 255,
  };
}

std::optional<SkColor4f> colorFromProp(const facebook::react::SharedColor &color) {
  if (!color) return std::nullopt;
  const auto components = facebook::react::colorComponentsFromColor(color);
  return SkColor4f{components.red, components.green, components.blue, components.alpha};
}

} // namespace rnwebos::svg
