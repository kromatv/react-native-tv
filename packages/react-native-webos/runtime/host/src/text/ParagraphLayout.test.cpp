#include "ParagraphLayout.h"

#include "../render/RasterTarget.test.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace rnwebos {

namespace {

using facebook::react::AttributedString;
using facebook::react::Float;
using facebook::react::ParagraphAttributes;
using facebook::react::TextAttributes;

constexpr Float kUnbounded = std::numeric_limits<Float>::infinity();
constexpr Float kFontSize = 20;
const std::string kSentence = "The quick brown fox jumps over the lazy dog and keeps running";

TextAttributes attributes(Float fontSize = kFontSize) {
  TextAttributes text;
  text.fontFamily = "DejaVu Sans";
  text.fontSize = fontSize;
  text.foregroundColor = facebook::react::colorFromRGBA(255, 255, 255, 255);
  return text;
}

AttributedString textOf(const std::string &string, const TextAttributes &text = attributes()) {
  AttributedString attributed;
  attributed.appendFragment({.string = string, .textAttributes = text, .parentShadowView = {}});
  return attributed;
}

ParagraphAttributes atMostLines(int lines, facebook::react::EllipsizeMode mode) {
  ParagraphAttributes paragraph;
  paragraph.maximumNumberOfLines = lines;
  paragraph.ellipsizeMode = mode;
  return paragraph;
}

Float lineHeightOf(const TextAttributes &text = attributes()) {
  return layOutParagraph(textOf("Hg", text), {}, kUnbounded, 1).size.height;
}

} // namespace

TEST_CASE("layOutParagraph keeps text with room on one line, as wide as its glyphs") {
  const auto shortText = layOutParagraph(textOf("Hello"), {}, kUnbounded, 1);
  const auto longText = layOutParagraph(textOf("Hello, world"), {}, kUnbounded, 1);

  CHECK(shortText.paragraph->lineNumber() == 1);
  CHECK(shortText.size.width > 2 * kFontSize);
  CHECK(shortText.size.width < 4 * kFontSize);
  CHECK(shortText.size.height >= kFontSize);
  CHECK(shortText.size.height < 2 * kFontSize);
  CHECK(longText.size.width > shortText.size.width);
  CHECK(longText.size.height == shortText.size.height);
  CHECK(shortText.attachments.empty());
}

TEST_CASE("layOutParagraph wraps at the width it is given, a line's height for each line") {
  const auto oneLine = layOutParagraph(textOf(kSentence), {}, kUnbounded, 1);

  const auto wrapped = layOutParagraph(textOf(kSentence), {}, 200, 1);

  const auto lines = wrapped.paragraph->lineNumber();
  CHECK(oneLine.paragraph->lineNumber() == 1);
  CHECK(lines >= 3);
  CHECK(wrapped.size.width <= 200);
  CHECK(wrapped.size.width > 100);
  CHECK(wrapped.size.height == doctest::Approx(lines * lineHeightOf()).epsilon(0.02));
}

TEST_CASE("layOutParagraph breaks at an explicit newline and never inside a word that fits") {
  const auto broken = layOutParagraph(textOf("one\ntwo"), {}, kUnbounded, 1);
  const auto narrow = layOutParagraph(textOf("ab cd"), {}, 40, 1);

  CHECK(broken.paragraph->lineNumber() == 2);
  CHECK(narrow.paragraph->lineNumber() == 2);
}

TEST_CASE("layOutParagraph stops at the maximum number of lines, with an ellipsis unless the text is clipped") {
  const auto all = layOutParagraph(textOf(kSentence), {}, 200, 1);
  const auto tail = layOutParagraph(textOf(kSentence), atMostLines(2, facebook::react::EllipsizeMode::Tail), 200, 1);
  const auto clip = layOutParagraph(textOf(kSentence), atMostLines(2, facebook::react::EllipsizeMode::Clip), 200, 1);
  const auto fits = layOutParagraph(textOf("Short"), atMostLines(2, facebook::react::EllipsizeMode::Tail), 200, 1);

  CHECK(all.paragraph->lineNumber() > 2);
  CHECK(tail.paragraph->lineNumber() == 2);
  CHECK(tail.paragraph->didExceedMaxLines());
  CHECK(tail.size.height == doctest::Approx(2 * lineHeightOf()).epsilon(0.02));
  CHECK(tail.size.width <= 200);
  CHECK(clip.paragraph->lineNumber() == 2);
  CHECK(clip.size.height == tail.size.height);
  CHECK(fits.paragraph->lineNumber() == 1);
  CHECK_FALSE(fits.paragraph->didExceedMaxLines());
}

TEST_CASE("layOutParagraph gives every line the lineHeight asked for, whatever the font's own") {
  auto tall = attributes();
  tall.lineHeight = 50;
  auto tight = attributes();
  tight.lineHeight = 12;

  const auto oneTall = layOutParagraph(textOf("Hg", tall), {}, kUnbounded, 1);
  const auto threeTall = layOutParagraph(textOf("one\ntwo\nthree", tall), {}, kUnbounded, 1);
  const auto twoTight = layOutParagraph(textOf("one\ntwo", tight), {}, kUnbounded, 1);

  CHECK(oneTall.size.height == 50);
  CHECK(threeTall.size.height == 150);
  CHECK(twoTight.size.height == 24);
  CHECK(oneTall.size.width == layOutParagraph(textOf("Hg"), {}, kUnbounded, 1).size.width);
}

TEST_CASE("layOutParagraph lays out in pixels at the scale and answers in points") {
  const auto atOne = layOutParagraph(textOf(kSentence), {}, 200, 1);
  const auto atTwo = layOutParagraph(textOf(kSentence), {}, 200, 2);

  CHECK(atTwo.paragraph->getHeight() == doctest::Approx(2 * atOne.paragraph->getHeight()).epsilon(0.05));
  CHECK(atTwo.size.width <= 200);
  CHECK(atTwo.size.height == doctest::Approx(atOne.size.height).epsilon(0.05));
  CHECK(std::ceil(atTwo.size.width * 2) == atTwo.size.width * 2);
  CHECK(std::ceil(atTwo.size.height * 2) == atTwo.size.height * 2);
}

TEST_CASE("the size layOutParagraph measures is the size of what it draws") {
  for (const Float scale : {Float{1}, Float{2}, Float{2} / 3, Float{1.5}}) {
    for (const Float room : {Float{137}, Float{200}, Float{333}, kUnbounded}) {
      CAPTURE(scale);
      CAPTURE(room);
      const auto measured = layOutParagraph(textOf(kSentence), {}, room, scale);

      const auto drawn = layOutParagraph(textOf(kSentence), {}, measured.size.width, scale);

      CHECK(drawn.paragraph->lineNumber() == measured.paragraph->lineNumber());
      CHECK(drawn.size.width == measured.size.width);
      CHECK(drawn.size.height == measured.size.height);
    }
  }
}

TEST_CASE("layOutParagraph's glyphs stay inside the size it measured") {
  const auto layout = layOutParagraph(textOf(kSentence), {}, 200, 1);
  const int width = static_cast<int>(layout.size.width);
  const int height = static_cast<int>(layout.size.height);
  constexpr int kMargin = 20;
  testing::RasterTarget target({.width = width + 2 * kMargin, .height = height + 2 * kMargin});

  layout.paragraph->paint(&target.canvas(), kMargin, kMargin);

  int inked = 0;
  PixelRect ink{.x = width + 2 * kMargin, .y = height + 2 * kMargin, .width = 0, .height = 0};
  for (int y = 0; y < height + 2 * kMargin; ++y) {
    for (int x = 0; x < width + 2 * kMargin; ++x) {
      if (SkColorGetA(target.at(x, y)) == 0) continue;
      ++inked;
      ink = {.x = std::min(ink.x, x), .y = std::min(ink.y, y), .width = std::max(ink.width, x), .height = std::max(ink.height, y)};
    }
  }
  const int right = ink.width;
  const int bottom = ink.height;
  CAPTURE(ink.x);
  CAPTURE(ink.y);
  CAPTURE(right);
  CAPTURE(bottom);
  CHECK(inked > 500);
  CHECK(ink.x >= kMargin - 1);
  CHECK(ink.y >= kMargin);
  CHECK(right <= kMargin + width);
  CHECK(bottom < kMargin + height);
  CHECK(bottom > kMargin + height - static_cast<int>(lineHeightOf()));
}

TEST_CASE("layOutParagraph makes room for an inline view and says where it sits, in points") {
  AttributedString text = textOf("Hello ");
  facebook::react::ShadowView inlineView{};
  inlineView.layoutMetrics.frame.size = {.width = 30, .height = 40};
  text.appendFragment({
      .string = AttributedString::Fragment::AttachmentCharacter(),
      .textAttributes = attributes(),
      .parentShadowView = inlineView,
  });
  const auto prefix = layOutParagraph(textOf("Hello "), {}, kUnbounded, 1);

  const auto atOne = layOutParagraph(text, {}, kUnbounded, 1);
  const auto atTwo = layOutParagraph(text, {}, kUnbounded, 2);

  REQUIRE(atOne.attachments.size() == 1);
  CHECK(atOne.attachments[0].size.width == doctest::Approx(30));
  CHECK(atOne.attachments[0].size.height == doctest::Approx(40));
  CHECK(atOne.attachments[0].origin.x > prefix.size.width - kFontSize);
  CHECK(atOne.size.width >= atOne.attachments[0].origin.x + 30);
  CHECK(atOne.size.height >= 40);
  REQUIRE(atTwo.attachments.size() == 1);
  CHECK(atTwo.attachments[0].size.width == doctest::Approx(30));
  CHECK(atTwo.attachments[0].size.height == doctest::Approx(40));
}

TEST_CASE("layOutParagraph draws a transformed fragment in its case, at its own font size") {
  auto upper = attributes();
  upper.textTransform = facebook::react::TextTransform::Uppercase;

  const auto lower = layOutParagraph(textOf("hello"), {}, kUnbounded, 1);
  const auto shouted = layOutParagraph(textOf("hello", upper), {}, kUnbounded, 1);
  const auto large = layOutParagraph(textOf("hello", attributes(40)), {}, kUnbounded, 1);

  CHECK(shouted.size.width == layOutParagraph(textOf("HELLO"), {}, kUnbounded, 1).size.width);
  CHECK(shouted.size.width > lower.size.width);
  CHECK(large.size.height > 1.8 * lower.size.height);
}

TEST_CASE("layOutParagraph lays out an empty text and a width of nothing without failing") {
  const auto empty = layOutParagraph(AttributedString{}, {}, 200, 1);
  const auto noRoom = layOutParagraph(textOf("Hello"), {}, 0, 1);
  const auto noScale = layOutParagraph(textOf("Hello"), {}, kUnbounded, 0);

  CHECK(empty.size.width == 0);
  CHECK(empty.size.height >= 0);
  CHECK(noRoom.paragraph->lineNumber() >= 1);
  CHECK(noScale.size.width == layOutParagraph(textOf("Hello"), {}, kUnbounded, 1).size.width);
}

} // namespace rnwebos
