#include "TextblockMarkup.h"

#include <Ecore_Evas.h>
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

facebook::react::AttributedString::Fragment fragmentOf(
    const std::string &text,
    const facebook::react::TextAttributes &attributes) {
  facebook::react::AttributedString::Fragment fragment;
  fragment.string = text;
  fragment.textAttributes = attributes;
  return fragment;
}

} // namespace

TEST_CASE("textblockContent of no text is the paragraph's style alone") {
  startEvas();

  const auto content = textblockContent({}, {}, 1);

  CHECK(content.style == "DEFAULT='font=Tizen font_size=14 color=#000000ff wrap=mixed align=auto'");
  CHECK(content.markup.empty());
  CHECK_FALSE(content.tightLineShift.has_value());
}

TEST_CASE("textblockContent leaves out an attachment") {
  startEvas();
  auto string = paragraphOf("text");
  string.appendFragment(fragmentOf(facebook::react::AttributedString::Fragment::AttachmentCharacter(), {}));

  const auto content = textblockContent(string, {}, 1);

  CHECK(content.markup == "<font=Tizen font_size=14 font_weight=normal color=#000000ff>text</>");
}

TEST_CASE("textblockContent shifts the paragraph by its first tight run") {
  static const bool started = ecore_evas_init() > 0;
  REQUIRE(started);
  facebook::react::TextAttributes small;
  small.lineHeight = 1;
  facebook::react::TextAttributes large = small;
  large.fontSize = 40;
  auto string = paragraphOf("small", small);
  string.appendFragment(fragmentOf("large", large));

  const auto content = textblockContent(string, {}, 1);
  const auto first = textblockContent(paragraphOf("small", small), {}, 1);
  const auto second = textblockContent(paragraphOf("large", large), {}, 1);

  REQUIRE(content.tightLineShift.has_value());
  CHECK(content.tightLineShift == first.tightLineShift);
  CHECK(second.tightLineShift != first.tightLineShift);
}

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
