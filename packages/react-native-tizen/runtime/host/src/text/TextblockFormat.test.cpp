#include "TextblockFormat.h"

#include <Ecore_Evas.h>
#include <doctest/doctest.h>

#include <string>

namespace rntizen {

namespace {

using facebook::react::colorFromRGBA;
using facebook::react::EllipsizeMode;
using facebook::react::FontStyle;
using facebook::react::FontWeight;
using facebook::react::ParagraphAttributes;
using facebook::react::TextAlignment;
using facebook::react::TextAttributes;
using facebook::react::TextDecorationLineType;
using facebook::react::TextDecorationStyle;

bool has(const std::string &format, const std::string &part) {
  return format.find(part) != std::string::npos;
}

void startEcoreEvas() {
  static const bool started = ecore_evas_init() > 0;
  REQUIRE(started);
}

} // namespace

TEST_CASE("fragmentFormat falls back to the system font at 14 points") {
  const auto run = fragmentFormat(TextAttributes{}, 2);

  CHECK(run.format == "font=Tizen font_size=28 font_weight=normal color=#000000ff");
  CHECK_FALSE(run.tightLineShift.has_value());
}

TEST_CASE("fragmentFormat escapes the spaces in a family name") {
  TextAttributes attributes;
  attributes.fontFamily = "One UI Sans";

  CHECK(has(fragmentFormat(attributes, 1).format, "font=One\\ UI\\ Sans "));
}

TEST_CASE("fragmentFormat names the weight class and slant") {
  TextAttributes attributes;
  attributes.fontWeight = FontWeight::Bold;
  attributes.fontStyle = FontStyle::Italic;

  const auto format = fragmentFormat(attributes, 1).format;

  CHECK(has(format, " font_weight=bold"));
  CHECK(has(format, " font_style=italic"));
}

TEST_CASE("fragmentFormat rounds a weight up to the next named one") {
  TextAttributes attributes;

  attributes.fontWeight = static_cast<FontWeight>(100);
  CHECK(has(fragmentFormat(attributes, 1).format, "font_weight=thin"));
  attributes.fontWeight = static_cast<FontWeight>(550);
  CHECK(has(fragmentFormat(attributes, 1).format, "font_weight=semibold"));
  attributes.fontWeight = static_cast<FontWeight>(900);
  CHECK(has(fragmentFormat(attributes, 1).format, "font_weight=black"));
}

TEST_CASE("fragmentFormat rounds the size to whole pixels, one at least") {
  TextAttributes attributes;

  attributes.fontSize = 20;
  CHECK(has(fragmentFormat(attributes, 1.5f).format, "font_size=30 "));
  attributes.fontSize = 0.1f;
  CHECK(has(fragmentFormat(attributes, 1).format, "font_size=1 "));
}

TEST_CASE("fragmentFormat writes the color with the opacity in its alpha") {
  TextAttributes attributes;
  attributes.foregroundColor = colorFromRGBA(255, 128, 0, 255);
  attributes.opacity = 0.5f;

  CHECK(has(fragmentFormat(attributes, 1).format, "color=#ff800080"));
}

TEST_CASE("fragmentFormat sets a lineHeight taller than the font's line as the line size") {
  startEcoreEvas();
  TextAttributes attributes;
  attributes.lineHeight = 100;
  attributes.alignment = TextAlignment::Center;

  const auto run = fragmentFormat(attributes, 2);

  CHECK(has(run.format, " align=center linesize=200"));
  CHECK_FALSE(run.tightLineShift.has_value());
}

TEST_CASE("fragmentFormat shrinks the line for a lineHeight below the font's and reports the shift") {
  startEcoreEvas();
  TextAttributes attributes;
  attributes.lineHeight = 1;

  const auto run = fragmentFormat(attributes, 2);

  CHECK(has(run.format, " linerelsize="));
  CHECK(has(run.format, "% linegap="));
  CHECK(run.tightLineShift.has_value());
}

TEST_CASE("fragmentFormat draws underlines, double underlines and strikes in the decoration color") {
  TextAttributes attributes;
  attributes.foregroundColor = colorFromRGBA(0, 0, 0, 255);
  attributes.textDecorationColor = colorFromRGBA(255, 0, 0, 255);
  attributes.textDecorationLineType = TextDecorationLineType::UnderlineStrikethrough;
  attributes.textDecorationStyle = TextDecorationStyle::Double;

  const auto format = fragmentFormat(attributes, 1).format;

  CHECK(has(format, " underline=double underline_color=#ff0000ff underline2_color=#ff0000ff"));
  CHECK(has(format, " strikethrough=on strikethrough_color=#ff0000ff"));
}

TEST_CASE("fragmentFormat backs the run with its background color") {
  TextAttributes attributes;
  attributes.backgroundColor = colorFromRGBA(0, 0, 255, 255);

  CHECK(has(fragmentFormat(attributes, 1).format, " backing=on backing_color=#0000ffff"));
}

TEST_CASE("fragmentFormat keeps the family when letter spacing names no app font") {
  TextAttributes attributes;
  attributes.fontFamily = "Unregistered";
  attributes.letterSpacing = 2;

  CHECK(has(fragmentFormat(attributes, 1).format, "font=Unregistered "));
}

TEST_CASE("paragraphFormat ellipsizes a single line where ellipsizeMode says") {
  ParagraphAttributes paragraph;
  paragraph.maximumNumberOfLines = 1;

  paragraph.ellipsizeMode = EllipsizeMode::Head;
  CHECK(has(paragraphFormat(paragraph, std::nullopt, 1), " ellipsis=0.0"));
  paragraph.ellipsizeMode = EllipsizeMode::Middle;
  CHECK(has(paragraphFormat(paragraph, std::nullopt, 1), " ellipsis=0.5"));
  paragraph.ellipsizeMode = EllipsizeMode::Tail;
  CHECK(has(paragraphFormat(paragraph, std::nullopt, 1), " ellipsis=1.0"));
  paragraph.ellipsizeMode = EllipsizeMode::Clip;
  CHECK_FALSE(has(paragraphFormat(paragraph, std::nullopt, 1), "ellipsis"));
}

TEST_CASE("paragraphFormat wraps several lines and aligns the paragraph") {
  ParagraphAttributes paragraph;
  paragraph.maximumNumberOfLines = 3;

  CHECK(paragraphFormat(paragraph, TextAlignment::Right, 1) ==
        "font=Tizen font_size=14 color=#000000ff wrap=mixed align=right");
}

} // namespace rntizen
