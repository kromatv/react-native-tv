#include "TextblockMarkup.h"

#include <Evas.h>
#include <doctest/doctest.h>

#include <string>

namespace rntizen {

namespace {

void startEvas() {
  static const bool started = evas_init() > 0;
  REQUIRE(started);
}

facebook::react::AttributedString paragraphOf(const std::string &text, facebook::react::TextAttributes attributes = {}) {
  facebook::react::AttributedString string;
  facebook::react::AttributedString::Fragment fragment;
  fragment.string = text;
  fragment.textAttributes = attributes;
  string.appendFragment(std::move(fragment));
  return string;
}

} // namespace

TEST_CASE("textblockContent escapes markup characters in the text") {
  startEvas();

  const auto content = textblockContent(paragraphOf("a<b> & c"), {}, 1);

  CHECK(content.markup.find("a&lt;b&gt; &amp; c</>") != std::string::npos);
}

TEST_CASE("textblockContent turns a newline into a line break") {
  startEvas();

  const auto content = textblockContent(paragraphOf("one\ntwo"), {}, 1);

  CHECK(content.markup.find("one<br/>two") != std::string::npos);
}

TEST_CASE("textblockContent wraps each run in its own format") {
  startEvas();
  auto string = paragraphOf("plain ");
  facebook::react::AttributedString::Fragment bold;
  bold.string = "bold";
  bold.textAttributes.fontWeight = facebook::react::FontWeight::Bold;
  string.appendFragment(std::move(bold));

  const auto content = textblockContent(string, {}, 1);

  CHECK(content.markup ==
        "<font=Tizen font_size=14 font_weight=normal color=#000000ff>plain </>"
        "<font=Tizen font_size=14 font_weight=bold color=#000000ff>bold</>");
}

TEST_CASE("textblockContent applies the text transform before escaping") {
  startEvas();
  facebook::react::TextAttributes attributes;
  attributes.textTransform = facebook::react::TextTransform::Uppercase;

  const auto content = textblockContent(paragraphOf("a&b", attributes), {}, 1);

  CHECK(content.markup.find(">A&amp;B</>") != std::string::npos);
}

TEST_CASE("textblockContent styles the paragraph with its first run's alignment") {
  startEvas();
  facebook::react::TextAttributes attributes;
  attributes.alignment = facebook::react::TextAlignment::Center;

  const auto content = textblockContent(paragraphOf("x", attributes), {}, 1);

  CHECK(content.style.starts_with("DEFAULT='font=Tizen"));
  CHECK(content.style.find("align=center'") != std::string::npos);
}

} // namespace rntizen
