#include "EvasTextMeasurer.h"

#include <doctest/doctest.h>

#include <Ecore_Evas.h>

#include <limits>
#include <string>

namespace rntizen {

namespace {

using facebook::react::AttributedString;
using facebook::react::Float;
using facebook::react::ParagraphAttributes;
using facebook::react::Size;

void initializeCanvases() {
  static const bool initialized = ecore_evas_init() > 0;
  REQUIRE(initialized);
}

AttributedString paragraph(const std::string &text, Float lineHeight = std::numeric_limits<Float>::quiet_NaN()) {
  facebook::react::TextAttributes attributes{};
  attributes.fontSize = 20;
  attributes.lineHeight = lineHeight;
  AttributedString string;
  string.appendFragment({.string = text, .textAttributes = attributes, .parentShadowView = {}});
  return string;
}

} // namespace

TEST_CASE("EvasTextMeasurer measures a line once for every width it fits") {
  initializeCanvases();
  const EvasTextMeasurer measurer;
  const ParagraphAttributes attributes{};

  const auto first = measurer.measure(paragraph("Hello"), attributes, {.width = 1000, .height = 1000}, 1);
  const auto wider = measurer.measure(paragraph("Hello"), attributes, {.width = 1500, .height = 1000}, 1);

  CHECK(first.width > 0);
  CHECK(first.height > 0);
  CHECK(wider == first);
}

TEST_CASE("EvasTextMeasurer wraps a paragraph and cuts it after its lines") {
  initializeCanvases();
  const EvasTextMeasurer measurer;
  const auto text = paragraph("one two three four five six seven eight nine ten eleven twelve");
  ParagraphAttributes twoLines{};
  twoLines.maximumNumberOfLines = 2;

  const auto line = measurer.measure(paragraph("one"), {}, {.width = 1000, .height = 1000}, 1);
  const auto wrapped = measurer.measure(text, {}, {.width = 80, .height = 1000}, 1);
  const auto clamped = measurer.measure(text, twoLines, {.width = 80, .height = 1000}, 1);

  CHECK(wrapped.height > 2 * line.height);
  CHECK(clamped.height < wrapped.height);
}

TEST_CASE("EvasTextMeasurer measures a line height below the font's own") {
  initializeCanvases();
  const EvasTextMeasurer measurer;

  const auto natural = measurer.measure(paragraph("Hello"), {}, {.width = 1000, .height = 1000}, 1);
  const auto tight = measurer.measure(paragraph("Hello", 10), {}, {.width = 1000, .height = 1000}, 1);

  CHECK(tight.height > 0);
  CHECK(tight.height < natural.height);
}

} // namespace rntizen
