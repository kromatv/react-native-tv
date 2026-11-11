#include "SvgPaint.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr auto kFill = static_cast<std::size_t>(PaintAttribute::Fill);
constexpr auto kStrokeWidth = static_cast<std::size_t>(PaintAttribute::StrokeWidth);
constexpr auto kStrokeDasharray = static_cast<std::size_t>(PaintAttribute::StrokeDasharray);

void checkColor(const SkColor4f &actual, float red, float green, float blue, float alpha) {
  CHECK(actual.fR == doctest::Approx(red));
  CHECK(actual.fG == doctest::Approx(green));
  CHECK(actual.fB == doctest::Approx(blue));
  CHECK(actual.fA == doctest::Approx(alpha));
}

} // namespace

TEST_CASE("colorFromArgb unpacks a processed color") {
  checkColor(colorFromArgb(0x80FF0000), 1, 0, 0, 128.0F / 255);
  checkColor(colorFromArgb(0xFF336699), 0.2F, 0.4F, 0.6F, 1);
}

TEST_CASE("brushFromProp reads a color, a reference and currentColor") {
  const auto solid = brushFromProp(folly::dynamic::object("type", 0)("payload", 0xFF00FF00));
  CHECK(solid.kind == Brush::Kind::Color);
  checkColor(solid.color, 0, 1, 0, 1);

  const auto reference = brushFromProp(folly::dynamic::object("type", 1)("brushRef", "ramp"));
  CHECK(reference.kind == Brush::Kind::Reference);
  CHECK(reference.reference == "ramp");

  CHECK(brushFromProp(folly::dynamic::object("type", 2)).kind == Brush::Kind::CurrentColor);
}

TEST_CASE("brushFromProp reads a bare color number, signed as Android packs it or not") {
  checkColor(brushFromProp(folly::dynamic(4278190335.0)).color, 0, 0, 1, 1);
  checkColor(brushFromProp(folly::dynamic(-16776961)).color, 0, 0, 1, 1);
}

TEST_CASE("brushFromProp paints nothing for none and for what it cannot read") {
  CHECK(brushFromProp(folly::dynamic(nullptr)).kind == Brush::Kind::None);
  CHECK(brushFromProp(folly::dynamic::object("type", 1)).kind == Brush::Kind::None);
  CHECK(brushFromProp(folly::dynamic::object("type", 3)).kind == Brush::Kind::None);
  CHECK(brushFromProp(folly::dynamic::object("payload", 1)).kind == Brush::Kind::None);
}

TEST_CASE("lengthFromProp reads numbers, units and percentages") {
  CHECK(lengthFromProp(folly::dynamic(2.5)) == Length{.value = 2.5F});
  CHECK(lengthFromProp(folly::dynamic("12")) == Length{.value = 12});
  CHECK(lengthFromProp(folly::dynamic(" +3px ")) == Length{.value = 3});
  CHECK(lengthFromProp(folly::dynamic("50%")) == Length{.value = 50, .percent = true});
  CHECK(lengthFromProp(folly::dynamic("1in")) == Length{.value = 90});
  CHECK(lengthFromProp(folly::dynamic("2pt")) == Length{.value = 2.5F});
}

TEST_CASE("lengthFromProp refuses what is no length") {
  CHECK_FALSE(lengthFromProp(folly::dynamic("wide")).has_value());
  CHECK_FALSE(lengthFromProp(folly::dynamic("3furlongs")).has_value());
  CHECK_FALSE(lengthFromProp(folly::dynamic(nullptr)).has_value());
}

TEST_CASE("a length resolves against its reference only as a percentage") {
  CHECK(Length{.value = 50, .percent = true}.resolve(200) == doctest::Approx(100));
  CHECK(Length{.value = 50}.resolve(200) == doctest::Approx(50));
}

TEST_CASE("a length in bounding-box units is a share of the box either way") {
  CHECK(Length{.value = 50, .percent = true}.fractionOf(200) == doctest::Approx(100));
  CHECK(Length{.value = 0.5F}.fractionOf(200) == doctest::Approx(100));
}

TEST_CASE("lengthsFromProp reads a list and leaves out what is no length") {
  const auto lengths = lengthsFromProp(folly::dynamic::array(4, "2", "x", "10%"));

  REQUIRE(lengths.size() == 3);
  CHECK(lengths[0] == Length{.value = 4});
  CHECK(lengths[1] == Length{.value = 2});
  CHECK(lengths[2] == Length{.value = 10, .percent = true});
}

TEST_CASE("lengthsFromProp reads one length alone, and nothing from null") {
  CHECK(lengthsFromProp(folly::dynamic(7)).size() == 1);
  CHECK(lengthsFromProp(folly::dynamic(nullptr)).empty());
}

TEST_CASE("paintAttributesFromPropList keeps the names it knows") {
  const auto attributes = paintAttributesFromPropList({"fill", "strokeDasharray", "fontSize"});

  CHECK(attributes.count() == 2);
  CHECK(attributes[kFill]);
  CHECK(attributes[kStrokeDasharray]);
}

TEST_CASE("inheritPaint takes from the parent what the node does not list") {
  Paint parent{.strokeWidth = {.value = 6}, .strokeDasharray = {{.value = 2}, {.value = 3}}};
  parent.fill.color = {1, 0, 0, 1};
  parent.listed.set(kFill).set(kStrokeWidth).set(kStrokeDasharray);
  Paint own{.strokeWidth = {.value = 2}};
  own.listed.set(kStrokeWidth);

  const auto resolved = inheritPaint(own, parent);

  CHECK(resolved.fill.color.fR == doctest::Approx(1));
  CHECK(resolved.strokeWidth.value == doctest::Approx(2));
  CHECK(resolved.strokeDasharray.size() == 2);
  CHECK(resolved.listed[kFill]);
  CHECK(resolved.listed[kStrokeWidth]);
}

TEST_CASE("inheritPaint keeps the node's default for what the parent does not list") {
  Paint parent{.strokeMiterlimit = 10};
  const auto resolved = inheritPaint(Paint{}, parent);

  CHECK(resolved.strokeMiterlimit == doctest::Approx(4));
  CHECK(resolved.listed.none());
}

} // namespace rnwebos::svg
