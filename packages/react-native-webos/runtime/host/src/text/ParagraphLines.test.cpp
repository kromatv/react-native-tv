#include "ParagraphLines.h"

#include "../render/RasterTarget.test.h"
#include "ParagraphLayout.h"

#include <doctest/doctest.h>

#include <cmath>
#include <limits>
#include <string>

namespace rnwebos {

namespace {

using facebook::react::AttributedString;
using facebook::react::Float;
using facebook::react::TextAttributes;

constexpr Float kUnbounded = std::numeric_limits<Float>::infinity();

AttributedString textOf(const std::string &string, Float fontSize = 20, facebook::react::TextTransform transform = {}) {
  TextAttributes attributes;
  attributes.fontFamily = "DejaVu Sans";
  attributes.fontSize = fontSize;
  attributes.textTransform = transform;
  AttributedString attributed;
  attributed.appendFragment({.string = string, .textAttributes = attributes, .parentShadowView = {}});
  return attributed;
}

} // namespace

TEST_CASE("measureParagraphLines gives one line its text, its box and a baseline inside it") {
  const auto text = textOf("Hello, world");
  const auto layout = layOutParagraph(text, {}, kUnbounded, 1);

  const auto lines = measureParagraphLines(text, {}, kUnbounded, 1);

  REQUIRE(lines.size() == 1);
  CHECK(lines[0].text == "Hello, world");
  CHECK(lines[0].frame.origin.y == 0);
  CHECK(lines[0].frame.size.width == doctest::Approx(layout.paragraph->getLongestLine()).epsilon(0.001));
  CHECK(lines[0].frame.size.height == doctest::Approx(layout.paragraph->getHeight()));
  CHECK(lines[0].ascender == doctest::Approx(layout.paragraph->getAlphabeticBaseline()));
  CHECK(lines[0].ascender > lines[0].frame.size.height / 2);
  CHECK(lines[0].ascender < lines[0].frame.size.height);
  CHECK(lines[0].descender > 0);
  CHECK(lines[0].capHeight > lines[0].xHeight);
  CHECK(lines[0].xHeight > 0);
}

TEST_CASE("the baseline measureParagraphLines reports is where the letters stand") {
  for (const Float fontSize : {Float{20}, Float{33}, Float{48}}) {
    CAPTURE(fontSize);
    AttributedString text = textOf("HHHH", fontSize);
    text.getFragments()[0].textAttributes.foregroundColor = facebook::react::colorFromRGBA(255, 255, 255, 255);
    const auto layout = layOutParagraph(text, {}, kUnbounded, 1);
    testing::RasterTarget target({.width = 300, .height = 120});
    layout.paragraph->paint(&target.canvas(), 0, 0);
    int inkBottom = -1;
    for (int y = 0; y < 120; ++y) {
      for (int x = 0; x < 300; ++x) {
        if (SkColorGetA(target.at(x, y)) > 128) inkBottom = y;
      }
    }

    const auto lines = measureParagraphLines(text, {}, kUnbounded, 1);

    REQUIRE(lines.size() == 1);
    CHECK(std::abs(lines[0].ascender - static_cast<Float>(inkBottom + 1)) < 1);
  }
}

TEST_CASE("measureParagraphLines stacks wrapped lines, each with the words it holds") {
  const auto lines = measureParagraphLines(textOf("one two three four five six seven eight"), {}, 120, 1);

  REQUIRE(lines.size() >= 3);
  std::string joined;
  Float top = 0;
  for (const auto &line : lines) {
    CHECK(line.frame.origin.y == doctest::Approx(top));
    CHECK(line.frame.size.width <= 120);
    CHECK_FALSE(line.text.empty());
    CHECK(line.text.back() != ' ');
    joined += (joined.empty() ? "" : " ") + line.text;
    top += line.frame.size.height;
  }
  CHECK(joined == "one two three four five six seven eight");
}

TEST_CASE("measureParagraphLines cuts a line at its characters, not its bytes, and in the case shown") {
  const auto accents = measureParagraphLines(textOf("élève à l'école 😀 fin"), {}, kUnbounded, 1);
  const auto upper = measureParagraphLines(
      textOf("élève", 20, facebook::react::TextTransform::Uppercase), {}, kUnbounded, 1);

  REQUIRE(accents.size() == 1);
  CHECK(accents[0].text == "élève à l'école 😀 fin");
  REQUIRE(upper.size() == 1);
  CHECK(upper[0].text == "ÉLÈVE");
}

TEST_CASE("measureParagraphLines answers in points whatever the scale, and a bigger font sits lower") {
  const auto atOne = measureParagraphLines(textOf("Baseline"), {}, kUnbounded, 1);
  const auto atTwoThirds = measureParagraphLines(textOf("Baseline"), {}, kUnbounded, Float{2} / 3);
  const auto big = measureParagraphLines(textOf("Baseline", 48), {}, kUnbounded, 1);

  REQUIRE(atOne.size() == 1);
  REQUIRE(atTwoThirds.size() == 1);
  REQUIRE(big.size() == 1);
  CHECK(atTwoThirds[0].ascender == doctest::Approx(atOne[0].ascender).epsilon(0.08));
  CHECK(big[0].ascender > 2 * atOne[0].ascender);
}

TEST_CASE("measureParagraphLines gives an empty text no line to align on") {
  CHECK(measureParagraphLines(AttributedString{}, {}, 200, 1).size() <= 1);
}

} // namespace rnwebos
