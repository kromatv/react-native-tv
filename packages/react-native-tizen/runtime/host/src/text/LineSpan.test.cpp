#include "LineSpan.h"

#include "Textblock.test.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

using testing::Textblock;

constexpr const char *kStyle = "font=Sans font_size=20 color=#000000ff wrap=word";

Evas_Coord lineBottom(const Textblock &paragraph, int line) {
  Evas_Coord y = 0;
  Evas_Coord height = 0;
  evas_object_textblock_line_number_geometry_get(paragraph.object(), line, nullptr, &y, nullptr, &height);
  return y + height;
}

} // namespace

TEST_CASE("lineSpan runs from the first line's top to the last line's bottom") {
  const Textblock paragraph(kStyle, "one<br/>two<br/>three", 200, 400);
  Evas_Coord top = 0;
  evas_object_textblock_line_number_geometry_get(paragraph.object(), 0, nullptr, &top, nullptr, nullptr);

  const auto span = lineSpan(paragraph.object(), 0);

  REQUIRE(paragraph.lineCount() == 3);
  CHECK(span.top == top);
  CHECK(span.bottom == lineBottom(paragraph, 2));
}

TEST_CASE("lineSpan stops at maxLines, or at the last line when there are fewer") {
  const Textblock paragraph(kStyle, "one<br/>two<br/>three", 200, 400);

  const auto clamped = lineSpan(paragraph.object(), 2);
  const auto roomy = lineSpan(paragraph.object(), 10);

  CHECK(clamped.bottom == lineBottom(paragraph, 1));
  CHECK(roomy.bottom == lineBottom(paragraph, 2));
}

} // namespace rntizen
