#include "SvgTextNode.h"

#include "SvgRaster.test.h"

#include <doctest/doctest.h>

#include <string>

namespace rnwebos::svg {

namespace {

constexpr int kWidth = 400;
constexpr int kHeight = 100;
constexpr float kBaseline = 70;

/// The columns that hold ink: the first, and one past the last. Both -1 on an empty picture.
struct Ink {
  int left{-1};
  int right{-1};
  int below{0};
};

Ink inkOf(const Raster &raster) {
  Ink ink;
  for (int x = 0; x < kWidth; ++x) {
    for (int y = 0; y < kHeight; ++y) {
      if (SkColorGetA(raster.at(x, y)) == 0) continue;
      if (ink.left < 0) ink.left = x;
      ink.right = x + 1;
      if (y > static_cast<int>(kBaseline) + 1) ++ink.below;
    }
  }
  return ink;
}

void write(TextNode &text, const std::string &content, float size = 40) {
  text.setContent(content);
  text.font.size = size;
  text.paint = filled(SK_ColorRED);
}

Ink drawn(TextNode &text) {
  RootNode root;
  root.setChildren({&text});
  Raster raster(kWidth, kHeight);
  raster.draw(root);
  return inkOf(raster);
}

} // namespace

TEST_CASE("a text draws its glyphs from its x to the right, standing on its y") {
  TextNode text;
  write(text, "HHHH");
  text.x = {{.value = 100}};
  text.y = {{.value = kBaseline}};

  const auto ink = drawn(text);

  CHECK(ink.left >= 100);
  CHECK(ink.left < 110);
  CHECK(ink.right > 180);
  CHECK(ink.below == 0);
}

TEST_CASE("a text draws in its fill, and nothing without content or with a size of zero") {
  TextNode text;
  write(text, "HHHH");
  text.x = {{.value = 100}};
  text.y = {{.value = kBaseline}};
  RootNode root;
  root.setChildren({&text});
  Raster raster(kWidth, kHeight);
  raster.draw(root);
  const auto ink = inkOf(raster);
  REQUIRE(ink.left >= 0);
  bool red = false;
  for (int y = 0; y < kHeight && !red; ++y) red = raster.at(ink.left + 4, y) == SK_ColorRED;
  CHECK(red);

  text.setContent("");
  CHECK(drawn(text).left == -1);

  write(text, "HHHH", 0);
  CHECK(drawn(text).left == -1);
}

TEST_CASE("textAnchor end puts a text before its x, and middle around it") {
  TextNode text;
  write(text, "HHHH");
  text.x = {{.value = 200}};
  text.y = {{.value = kBaseline}};
  const int width = drawn(text).right - 200;
  REQUIRE(width > 60);

  text.font.anchor = TextAnchor::End;
  const auto end = drawn(text);
  CHECK(end.right <= 201);
  CHECK(end.left < 200 - width + 12);

  text.font.anchor = TextAnchor::Middle;
  const auto middle = drawn(text);
  CHECK(middle.left < 200 - width / 2 + 12);
  CHECK(middle.right > 200 + width / 2 - 12);
}

TEST_CASE("a span goes on where its text ended, unless it has an x of its own") {
  TextNode text;
  write(text, "HH");
  text.x = {{.value = 20}};
  text.y = {{.value = kBaseline}};
  const int alone = drawn(text).right;

  TextNode span;
  write(span, "HH");
  text.setChildren({&span});
  const int continued = drawn(text).right;
  CHECK(continued > alone + 30);

  span.x = {{.value = 300}};
  const auto moved = drawn(text);
  CHECK(moved.left < 30);
  CHECK(moved.right > 330);

  span.hidden = true;
  CHECK(drawn(text).right == alone);
}

TEST_CASE("a text anchored at its end counts the spans up to one with an x of its own") {
  TextNode text;
  write(text, "HH");
  text.x = {{.value = 300}};
  text.y = {{.value = kBaseline}};
  text.font.anchor = TextAnchor::End;
  TextNode span;
  write(span, "HH");
  text.setChildren({&span});

  const auto both = drawn(text);
  CHECK(both.right <= 301);

  TextNode apart;
  write(apart, "HH");
  apart.x = {{.value = 100}};
  text.setChildren({&span, &apart});
  const auto three = drawn(text);
  CHECK(three.right <= 301);
  CHECK(three.left < 100);
}

TEST_CASE("dx and dy shift a text's pen") {
  TextNode text;
  write(text, "HH");
  text.x = {{.value = 20}};
  text.y = {{.value = 40}};
  const int left = drawn(text).left;

  text.dx = {{.value = 100}};
  text.dy = {{.value = 30}};
  const auto shifted = drawn(text);

  CHECK(shifted.left == left + 100);
  CHECK(shifted.below == 0);
}

} // namespace rnwebos::svg
