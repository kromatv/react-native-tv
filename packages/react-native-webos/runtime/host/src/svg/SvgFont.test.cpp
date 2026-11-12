#include "SvgFont.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

TEST_CASE("fontAttributesFromProp reads what extractFont sends") {
  const auto attributes = fontAttributesFromProp(folly::dynamic::object("fontFamily", "Museo Sans")("fontSize", 24)(
      "fontWeight", "bold")("fontStyle", "italic")("textAnchor", "middle"));

  CHECK(attributes.family == "Museo Sans");
  CHECK(attributes.size == doctest::Approx(24));
  CHECK_FALSE(attributes.relativeSize);
  CHECK(attributes.weight == 700);
  CHECK(attributes.italic == true);
  CHECK(attributes.anchor == TextAnchor::Middle);
}

TEST_CASE("fontAttributesFromProp leaves unset what the prop does not carry") {
  const auto attributes = fontAttributesFromProp(folly::dynamic::object("fontWeight", 300));

  CHECK_FALSE(attributes.family.has_value());
  CHECK_FALSE(attributes.size.has_value());
  CHECK(attributes.weight == 300);
  CHECK_FALSE(fontAttributesFromProp(folly::dynamic(nullptr)).weight.has_value());
}

TEST_CASE("a font size in em or percent is a multiple of the inherited size") {
  const Font inherited{.size = 20};
  const auto sized = [&inherited](const char *size) {
    return fontAttributesFromProp(folly::dynamic::object("fontSize", size)).over(inherited).size;
  };

  CHECK(sized("1.5em") == doctest::Approx(30));
  CHECK(sized("50%") == doctest::Approx(10));
  CHECK(sized("18px") == doctest::Approx(18));
}

TEST_CASE("a node's font properties go over the inherited ones") {
  const Font inherited{.family = "Noto Sans", .size = 20, .weight = 700, .anchor = TextAnchor::End};

  const auto own = fontAttributesFromProp(folly::dynamic::object("fontSize", 12)("fontWeight", "normal"));
  const auto font = own.over(inherited);

  CHECK(font.family == "Noto Sans");
  CHECK(font.size == doctest::Approx(12));
  CHECK(font.weight == 400);
  CHECK(font.anchor == TextAnchor::End);
}

TEST_CASE("fontAttributesFromProp reads a weight written as a number in a string, and every text anchor") {
  const auto weight = [](const folly::dynamic &value) {
    return fontAttributesFromProp(folly::dynamic::object("fontWeight", value)).weight;
  };
  CHECK(weight("600") == 600);
  CHECK_FALSE(weight("chunky").has_value());
  CHECK_FALSE(weight(true).has_value());

  const auto anchor = [](const folly::dynamic &value) {
    return fontAttributesFromProp(folly::dynamic::object("textAnchor", value)).anchor;
  };
  CHECK(anchor("start") == TextAnchor::Start);
  CHECK(anchor("end") == TextAnchor::End);
  CHECK_FALSE(anchor("justify").has_value());
  CHECK_FALSE(anchor(3).has_value());
}

TEST_CASE("a node that sets every font property replaces the inherited ones, and one that sets none keeps them") {
  const Font inherited{.family = "Noto Sans", .size = 20, .weight = 700, .italic = false, .anchor = TextAnchor::End};

  CHECK(FontAttributes{}.over(inherited) == inherited);

  const auto all = fontAttributesFromProp(folly::dynamic::object("fontFamily", "DejaVu Sans")("fontSize", 30)(
      "fontWeight", 300)("fontStyle", "italic")("textAnchor", "start"));
  CHECK(all.over(inherited) ==
        Font{.family = "DejaVu Sans", .size = 30, .weight = 300, .italic = true, .anchor = TextAnchor::Start});
}

} // namespace rnwebos::svg
