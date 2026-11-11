#include "PreparedParagraph.h"

#include "../render/RasterTarget.test.h"
#include "ParagraphLayout.h"

#include <doctest/doctest.h>

#include <string>

namespace rnwebos {

namespace {

using facebook::react::AttributedString;
using facebook::react::Float;
using facebook::react::LayoutConstraints;
using facebook::react::TextAlignment;
using facebook::react::TextAttributes;

constexpr int kRoom = 400;
constexpr int kHeight = 120;
const std::string kSentence = "The quick brown fox jumps over the lazy dog";

AttributedString textOf(const std::string &string, TextAlignment alignment) {
  TextAttributes text;
  text.fontFamily = "DejaVu Sans";
  text.fontSize = 20;
  text.alignment = alignment;
  text.foregroundColor = facebook::react::colorFromRGBA(255, 255, 255, 255);
  AttributedString attributed;
  attributed.appendFragment({.string = string, .textAttributes = text, .parentShadowView = {}});
  return attributed;
}

LayoutConstraints widthBetween(Float minimum, Float maximum) {
  return {.minimumSize = {.width = minimum, .height = 0}, .maximumSize = {.width = maximum, .height = 1000}};
}

bool sameInk(const testing::RasterTarget &left, const testing::RasterTarget &right, int &inked) {
  inked = 0;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kRoom; ++x) {
      if (left.at(x, y) != right.at(x, y)) return false;
      if (SkColorGetA(left.at(x, y)) != 0) ++inked;
    }
  }
  return true;
}

} // namespace

TEST_CASE("prepareParagraph records what the paragraph draws laid out in the box it measured") {
  for (const auto alignment : {TextAlignment::Left, TextAlignment::Center, TextAlignment::Right}) {
    for (const std::string &string : {std::string("Hello"), kSentence}) {
      CAPTURE(static_cast<int>(alignment));
      CAPTURE(string);
      const auto text = textOf(string, alignment);

      const auto prepared = prepareParagraph(text, {}, widthBetween(0, 240), 1);

      const auto inBox = layOutParagraph(text, {}, prepared->measurement.size.width, 1);
      testing::RasterTarget recorded({.width = kRoom, .height = kHeight});
      testing::RasterTarget painted({.width = kRoom, .height = kHeight});
      prepared->picture->playback(&recorded.canvas());
      inBox.paragraph->paint(&painted.canvas(), 0, 0);
      int inked = 0;
      CHECK(prepared->measurement.size == inBox.size);
      CHECK(sameInk(recorded, painted, inked));
      CHECK(inked > 100);
    }
  }
}

TEST_CASE("prepareParagraph centers text in a box wider than its line when the box may not shrink") {
  const auto text = textOf("Hello", TextAlignment::Center);

  const auto shrunk = prepareParagraph(text, {}, widthBetween(0, kRoom), 1);
  const auto fixed = prepareParagraph(text, {}, widthBetween(kRoom, kRoom), 1);

  testing::RasterTarget left({.width = kRoom, .height = kHeight});
  testing::RasterTarget middle({.width = kRoom, .height = kHeight});
  shrunk->picture->playback(&left.canvas());
  fixed->picture->playback(&middle.canvas());
  const auto firstInk = [](const testing::RasterTarget &target) {
    for (int x = 0; x < kRoom; ++x) {
      for (int y = 0; y < kHeight; ++y) {
        if (SkColorGetA(target.at(x, y)) != 0) return x;
      }
    }
    return kRoom;
  };
  CHECK(shrunk->measurement.size.width < 80);
  CHECK(fixed->measurement.size.width == shrunk->measurement.size.width);
  CHECK(firstInk(left) < 4);
  CHECK(firstInk(middle) > kRoom / 2 - 40);
  CHECK(firstInk(middle) < kRoom / 2);
}

TEST_CASE("prepareParagraph keeps the scale it laid out at and where an inline view sits") {
  AttributedString text = textOf("Hello ", TextAlignment::Left);
  facebook::react::ShadowView attachment;
  attachment.layoutMetrics.frame.size = {.width = 30, .height = 20};
  text.appendFragment(
      {.string = AttributedString::Fragment::AttachmentCharacter(), .textAttributes = {}, .parentShadowView = attachment});

  const auto prepared = prepareParagraph(text, {}, widthBetween(0, kRoom), Float{2} / 3);
  const auto unscaled = prepareParagraph(text, {}, widthBetween(0, kRoom), 0);

  CHECK(prepared->scale == Float{2} / 3);
  CHECK(unscaled->scale == 1);
  REQUIRE(prepared->measurement.attachments.size() == 1);
  CHECK(prepared->measurement.attachments[0].frame.size.width == doctest::Approx(30));
  CHECK(prepared->measurement.attachments[0].frame.origin.x > 30);
}

} // namespace rnwebos
