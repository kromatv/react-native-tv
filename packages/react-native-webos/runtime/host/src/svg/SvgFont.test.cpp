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

} // namespace rnwebos::svg
