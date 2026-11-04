#include "LineClamp.h"

#include "Textblock.test.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

namespace {

using testing::Textblock;

constexpr const char *kWordStyle = "font=Sans font_size=20 color=#000000ff wrap=word";
constexpr const char *kMixedStyle = "font=Sans font_size=20 color=#000000ff wrap=mixed";
constexpr const char *kText = "one two three four five six seven eight nine ten eleven twelve";
constexpr int kWidth = 100;
constexpr int kHeight = 400;

} // namespace

TEST_CASE("clampLines cuts the text after its lines and ends it with an ellipsis") {
  const Textblock paragraph(kWordStyle, kText, kWidth, kHeight);

  clampLines(paragraph.object(), 2, true);

  const auto text = paragraph.markup();
  CHECK(text.size() < std::string(kText).size());
  CHECK(text.ends_with("&hellip;"));
  CHECK(text.starts_with("one"));
}

TEST_CASE("clampLines without an ellipsis only cuts") {
  const Textblock paragraph(kWordStyle, kText, kWidth, kHeight);

  clampLines(paragraph.object(), 1, false);

  const auto text = paragraph.markup();
  CHECK(text.size() < std::string(kText).size());
  CHECK_FALSE(text.ends_with("&hellip;"));
}

TEST_CASE("clampLines leaves a paragraph that already fits, or one clamped to no lines") {
  const Textblock fitting(kWordStyle, kText, kWidth, kHeight);
  const Textblock unclamped(kWordStyle, kText, kWidth, kHeight);

  clampLines(fitting.object(), 100, true);
  clampLines(unclamped.object(), 0, true);

  CHECK(fitting.markup() == kText);
  CHECK(unclamped.markup() == kText);
}

TEST_CASE("clampLines drops the spaces the last line ended with") {
  const Textblock paragraph(kWordStyle, "aaaa      bbbb cccc", kWidth, kHeight);

  clampLines(paragraph.object(), 1, false);

  CHECK(paragraph.markup() == "aaaa");
}

TEST_CASE("clampLines shortens a full last line until its ellipsis fits on it") {
  const Textblock paragraph(kMixedStyle, std::string(60, 'w'), kWidth, kHeight);

  clampLines(paragraph.object(), 2, true);

  CHECK(paragraph.lineCount() == 2);
  CHECK(paragraph.markup().ends_with("w&hellip;"));
}

TEST_CASE("clampLines puts the ellipsis after a line break that ends the last line") {
  const Textblock paragraph(kWordStyle, "one<br/><br/>three", kWidth, kHeight);

  clampLines(paragraph.object(), 2, true);

  CHECK(paragraph.markup() == "one<br/>&hellip;");
}

TEST_CASE("clampLines leaves only the ellipsis when the one line it keeps is blank") {
  const Textblock paragraph(kWordStyle, "<br/>two", kWidth, kHeight);

  clampLines(paragraph.object(), 1, true);

  CHECK(paragraph.markup() == "&hellip;");
}

} // namespace rntizen
