#include "SvgPaint.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <string_view>
#include <utility>

namespace rntizen::svg {

namespace {

constexpr int kSolidColorBrush = 0;
constexpr int kReferenceBrush = 1;
constexpr int kCurrentColorBrush = 2;

constexpr std::array<std::pair<std::string_view, PaintAttribute>, 9> kPropNames{{
    {"fill", PaintAttribute::Fill},
    {"fillOpacity", PaintAttribute::FillOpacity},
    {"fillRule", PaintAttribute::FillRule},
    {"stroke", PaintAttribute::Stroke},
    {"strokeOpacity", PaintAttribute::StrokeOpacity},
    {"strokeWidth", PaintAttribute::StrokeWidth},
    {"strokeLinecap", PaintAttribute::StrokeLinecap},
    {"strokeLinejoin", PaintAttribute::StrokeLinejoin},
    {"strokeMiterlimit", PaintAttribute::StrokeMiterlimit},
}};

constexpr std::array<std::pair<std::string_view, double>, 7> kUnitsInUserUnits{{
    {"", 1},
    {"px", 1},
    {"pt", 1.25},
    {"pc", 15},
    {"mm", 3.543307},
    {"cm", 35.43307},
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
    case Count:
      return;
  }
}

std::optional<Rgba> rgbaFromNumber(const folly::dynamic &value) {
  if (!value.isNumber()) return std::nullopt;
  return rgbaFromArgb(static_cast<std::uint32_t>(static_cast<std::int64_t>(value.asDouble())));
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
  double value = 0;
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

double Length::resolve(double reference) const {
  return percent ? value * reference / 100 : value;
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
  if (const auto color = rgbaFromNumber(value)) return {.kind = Color, .color = *color};
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
  const auto color = payload == nullptr ? std::nullopt : rgbaFromNumber(*payload);
  if (!color) return {};
  return {.kind = Color, .color = *color};
}

std::optional<Length> lengthFromProp(const folly::dynamic &value) {
  if (value.isNumber()) return Length{.value = value.asDouble()};
  if (value.isString()) return lengthFromString(value.getString());
  return std::nullopt;
}

Rgba rgbaFromArgb(std::uint32_t argb) {
  return {
      .red = static_cast<double>((argb >> 16) & 0xff) / 255,
      .green = static_cast<double>((argb >> 8) & 0xff) / 255,
      .blue = static_cast<double>(argb & 0xff) / 255,
      .alpha = static_cast<double>((argb >> 24) & 0xff) / 255,
  };
}

std::optional<Rgba> rgbaFromColor(const facebook::react::SharedColor &color) {
  if (!color) return std::nullopt;
  const auto components = facebook::react::colorComponentsFromColor(color);
  return Rgba{.red = components.red, .green = components.green, .blue = components.blue, .alpha = components.alpha};
}

} // namespace rntizen::svg
