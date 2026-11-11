#include "TextInputLayer.h"

#include "../intl/IcuData.h"

#include <doctest/doctest.h>

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>

#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

#include <chrono>
#include <string>
#include <thread>

namespace rnwebos {

namespace {

using facebook::react::AttributedString;
using facebook::react::ParagraphAttributes;
using facebook::react::TextAlignment;

constexpr PixelRect kBox{.x = 10, .y = 20, .width = 200, .height = 90};
constexpr int kCaretWidth = 2;
constexpr SkColor4f kCaretColor = SkColors::kRed;
constexpr SkColor4f kSelectionColor = SkColors::kGreen;
constexpr auto kBlink = std::chrono::milliseconds(500);

AttributedString textOf(const std::string &string, TextAlignment alignment) {
  auto attributes = facebook::react::TextAttributes::defaultTextAttributes();
  attributes.fontSize = 20;
  attributes.alignment = alignment;
  attributes.foregroundColor = facebook::react::whiteColor();
  AttributedString text;
  text.appendFragment({.string = string, .textAttributes = attributes, .parentShadowView = {}});
  return text;
}

/// A text input's content in a box of a scene with no window, drawn into a
/// bitmap when a test looks at pixels.
class Field {
 public:
  Field() {
    static const bool hasIcuData = (useIcuData(RN_WEBOS_TEST_ICU_DATA_DIR), true);
    REQUIRE(hasIcuData);
    layer.layout(kBox, 1, kCaretWidth);
  }

  void show(const std::string &text, std::size_t caret, bool multiline = false, TextAlignment alignment = TextAlignment::Left) {
    ParagraphAttributes attributes;
    attributes.maximumNumberOfLines = multiline ? 0 : 1;
    attributes.ellipsizeMode = facebook::react::EllipsizeMode::Clip;
    layer.setText(textOf(text, alignment), attributes, multiline);
    layer.setSelection(caret, 0, 0, kSelectionColor);
  }

  PixelRect caretAt(std::size_t offset) {
    layer.setSelection(offset, 0, 0, kSelectionColor);
    return layer.caretRect();
  }

  SkColor pixel(int x, int y) {
    const auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(240, 140));
    scene.drawInto(*surface->getCanvas());
    layer.draw(*surface->getCanvas());
    SkPixmap pixels;
    REQUIRE(surface->peekPixels(&pixels));
    return pixels.getColor(x, y);
  }

  Scene scene{PixelSize{.width = 240, .height = 140}};
  TextInputLayer layer{scene};
};

} // namespace

TEST_CASE_FIXTURE(Field, "the caret starts at the left of the box and moves right with each character") {
  show("abc", 0);

  const auto start = caretAt(0);
  const auto middle = caretAt(1);
  const auto end = caretAt(3);

  CHECK(start.x == kBox.x);
  CHECK(middle.x > start.x);
  CHECK(end.x > middle.x);
  CHECK(start.width == kCaretWidth);
  CHECK(start.height > 0);
}

TEST_CASE_FIXTURE(Field, "a space typed at the end of the text moves the caret on") {
  show("ab", 2);
  const auto afterLetter = layer.caretRect();

  show("ab ", 3);

  CHECK(layer.caretRect().x > afterLetter.x);
}

TEST_CASE_FIXTURE(Field, "a single line is centered vertically in a taller box, or put where textAlignVertical says") {
  show("abc", 0);
  const auto centered = layer.caretRect();

  layer.setVerticalAlignment(facebook::react::TextAlignmentVertical::Top);
  const auto top = layer.caretRect();
  layer.setVerticalAlignment(facebook::react::TextAlignmentVertical::Bottom);
  const auto bottom = layer.caretRect();

  CHECK(top.y == kBox.y);
  CHECK(centered.y > top.y);
  CHECK(bottom.y > centered.y);
  CHECK(bottom.y + bottom.height == kBox.y + kBox.height);
}

TEST_CASE_FIXTURE(Field, "a short single line is aligned inside its box") {
  show("abc", 3, false, TextAlignment::Right);
  const auto right = layer.caretRect();
  show("abc", 0, false, TextAlignment::Center);
  const auto center = layer.caretRect();

  CHECK(right.x + right.width == kBox.x + kBox.width);
  CHECK(center.x > kBox.x + kBox.width / 4);
  CHECK(center.x < kBox.x + kBox.width / 2);
}

TEST_CASE_FIXTURE(Field, "a single line longer than its box scrolls sideways to keep the caret in the box") {
  const std::string text = "a line of text much too long for a box two hundred pixels wide";
  show(text, text.size());

  const auto end = layer.caretRect();
  const auto start = caretAt(0);

  CHECK(layer.textSize().width > kBox.width);
  CHECK(end.x + end.width == kBox.x + kBox.width);
  CHECK(start.x == kBox.x);
}

TEST_CASE_FIXTURE(Field, "a multiline text wraps at the box's width and the caret follows it down") {
  const std::string text = "a line of text much too long for a box two hundred pixels wide";
  show(text, 0, true);

  const auto start = layer.caretRect();
  const auto end = caretAt(text.size());

  CHECK(layer.textSize().width == kBox.width);
  CHECK(layer.textSize().height > 2 * start.height);
  CHECK(end.y > start.y);
  CHECK(end.x < kBox.x + kBox.width);
}

TEST_CASE_FIXTURE(Field, "after a newline the caret is at the start of the next line") {
  show("ab\n", 2, true);
  const auto beforeNewline = layer.caretRect();

  const auto afterNewline = caretAt(3);

  CHECK(afterNewline.x == kBox.x);
  CHECK(afterNewline.y >= beforeNewline.y + beforeNewline.height - 1);
}

TEST_CASE_FIXTURE(Field, "the caret moves a line up or down to the nearest character, and off no end of the text") {
  show("aaaa\naa\naaaa", 7, true);

  CHECK(layer.caretOnLine(-1) == std::optional<std::size_t>(2));
  CHECK(layer.caretOnLine(1) == std::optional<std::size_t>(10));

  layer.setSelection(4, 0, 0, kSelectionColor);
  CHECK(layer.caretOnLine(-1) == std::nullopt);
  CHECK(layer.caretOnLine(1) == std::optional<std::size_t>(7));

  layer.setSelection(12, 0, 0, kSelectionColor);
  CHECK(layer.caretOnLine(1) == std::nullopt);
}

TEST_CASE_FIXTURE(Field, "a single line has no line to move the caret to") {
  show("abc", 1);

  CHECK(layer.caretOnLine(1) == std::nullopt);
  CHECK(layer.caretOnLine(-1) == std::nullopt);
}

TEST_CASE_FIXTURE(Field, "a text taller than its box scrolls down to keep the caret in the box") {
  show("1\n2\n3\n4\n5\n6\n7\n8", 15, true);

  const auto caret = layer.caretRect();

  CHECK(layer.textSize().height > kBox.height);
  CHECK(caret.y + caret.height == kBox.y + kBox.height);
  CHECK(caretAt(0).y == kBox.y);
}

TEST_CASE_FIXTURE(Field, "the selection is painted behind the text, in the content box only") {
  show("mmmm", 4, false);
  layer.setVerticalAlignment(facebook::react::TextAlignmentVertical::Top);
  const auto caret = layer.caretRect();

  layer.setSelection(4, 0, 4, kSelectionColor);

  CHECK(pixel(kBox.x + 1, caret.y + 1) == kSelectionColor.toSkColor());
  CHECK(pixel(caret.x + 8, caret.y + 1) == SK_ColorBLACK);
  CHECK(pixel(kBox.x - 2, caret.y + 1) == SK_ColorBLACK);
}

TEST_CASE_FIXTURE(Field, "the caret is drawn while shown, goes dark half a second later, and is not drawn when hidden") {
  show("ab", 2);
  const auto caret = layer.caretRect();
  const auto x = caret.x + 1;
  const auto y = caret.y + 1;
  CHECK(pixel(x, y) == SK_ColorBLACK);

  layer.setCaret(true, kCaretColor);
  CHECK(pixel(x, y) == kCaretColor.toSkColor());

  std::this_thread::sleep_for(kBlink + std::chrono::milliseconds(30));
  MainLoop::get().iterate();
  CHECK(pixel(x, y) == SK_ColorBLACK);

  layer.setSelection(1, 0, 0, kSelectionColor);
  CHECK(pixel(layer.caretRect().x + 1, y) == kCaretColor.toSkColor());

  layer.setCaret(false, kCaretColor);
  CHECK(pixel(layer.caretRect().x + 1, y) == SK_ColorBLACK);
}

} // namespace rnwebos
