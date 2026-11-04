#include "TextInputShadowNode.h"

#include "Components.test.h"

#include <doctest/doctest.h>

#include <react/renderer/attributedstring/AttributedStringBox.h>
#include <react/renderer/core/LayoutContext.h>

#include <folly/dynamic.h>

#include <cmath>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace rntizen {

namespace {

using facebook::react::AttributedString;
using facebook::react::AttributedStringBox;
using facebook::react::Float;
using facebook::react::LayoutContext;
using facebook::react::Size;
using facebook::react::TextInputState;
using testing::Components;
using testing::kRoom;
using testing::RecordingMeasurer;

constexpr int kTreeEventCount = 3;
constexpr int kTypedEventCount = kTreeEventCount + 1;
constexpr Float kFieldWidth = 200;

Size lineOf(std::string_view text) {
  return {
      .width = static_cast<Float>(text.size()) * RecordingMeasurer::kCharacterWidth,
      .height = RecordingMeasurer::kLineHeight};
}

std::shared_ptr<TextInputShadowNode> laidOut(std::shared_ptr<facebook::react::ShadowNode> node) {
  auto textInput = std::dynamic_pointer_cast<TextInputShadowNode>(std::move(node));
  REQUIRE(textInput != nullptr);
  textInput->layoutTree(LayoutContext{}, kRoom);
  return textInput;
}

Size frameSize(const TextInputShadowNode &node) {
  return node.getLayoutMetrics().frame.size;
}

std::string stateText(const TextInputShadowNode &node) {
  return node.getStateData().attributedStringBox.getValue().getString();
}

class Field {
 public:
  Field()
      : field_(laidOut(components_.create(
            "AndroidTextInput",
            folly::dynamic::object("text", "hello")("mostRecentEventCount", kTreeEventCount)("width", kFieldWidth)))) {}

  std::shared_ptr<TextInputShadowNode> typedInto(folly::dynamic props, int eventCount) {
    AttributedString typed;
    typed.appendFragment({.string = "hello world", .textAttributes = {}, .parentShadowView = {}});
    const auto &state = field_->getStateData();
    const auto data = std::make_shared<const TextInputState>(
        AttributedStringBox(typed), state.reactTreeAttributedString, state.paragraphAttributes, eventCount);
    return laidOut(components_.update(*field_, std::move(props), data));
  }

  const RecordingMeasurer &measurer() const {
    return components_.measurer();
  }

 private:
  Components components_;
  std::shared_ptr<TextInputShadowNode> field_;
};

} // namespace

TEST_CASE("a single-line input measures its text at an unbounded width and clamps it to its box") {
  Components components;
  const std::string text = "a line of text too long for its field";

  const auto input = laidOut(components.create("AndroidTextInput", folly::dynamic::object("text", text)));

  CHECK(components.measurer().measured == text);
  CHECK(std::isinf(components.measurer().maximumWidth));
  CHECK(frameSize(*input) == Size{.width = kRoom.maximumSize.width, .height = RecordingMeasurer::kLineHeight});
}

TEST_CASE("a multiline input measures its text within the width it has") {
  Components components;

  const auto input =
      laidOut(components.create("AndroidTextInput", folly::dynamic::object("text", "hello")("multiline", true)));

  CHECK(components.measurer().maximumWidth == doctest::Approx(kRoom.maximumSize.width));
  CHECK(frameSize(*input) == lineOf("hello"));
}

TEST_CASE("an empty input measures its placeholder") {
  Components components;

  const auto input = laidOut(components.create("AndroidTextInput", folly::dynamic::object("placeholder", "Search")));

  CHECK(components.measurer().measured == "Search");
  CHECK(frameSize(*input) == lineOf("Search"));
}

TEST_CASE("an empty input with no placeholder is one line high and has no width") {
  Components components;

  const auto input = laidOut(components.create("AndroidTextInput", folly::dynamic::object()));

  CHECK(frameSize(*input) == lineOf(""));
}

TEST_CASE("layout keeps the tree's text in the state, with the props' event count") {
  Components components;

  const auto input = laidOut(components.create(
      "AndroidTextInput", folly::dynamic::object("text", "hello")("mostRecentEventCount", kTreeEventCount)));

  CHECK(stateText(*input) == "hello");
  CHECK(input->getStateData().reactTreeAttributedString.getString() == "hello");
  CHECK(input->getStateData().mostRecentEventCount == kTreeEventCount);
}

TEST_CASE_FIXTURE(Field, "while the tree's text has not changed, the input keeps and measures the text typed into it") {
  const auto input = typedInto(folly::dynamic::object(), kTreeEventCount);

  CHECK(stateText(*input) == "hello world");
  CHECK(input->getStateData().mostRecentEventCount == kTreeEventCount);
  CHECK(measurer().measured == "hello world");
}

TEST_CASE_FIXTURE(Field, "a new tree text with an older event count never overwrites the typed text") {
  const auto input =
      typedInto(folly::dynamic::object("text", "bye")("mostRecentEventCount", kTreeEventCount), kTypedEventCount);

  CHECK(stateText(*input) == "hello world");
  CHECK(input->getStateData().mostRecentEventCount == kTypedEventCount);
}

TEST_CASE_FIXTURE(Field, "a new tree text with a newer event count replaces the typed text") {
  const auto input =
      typedInto(folly::dynamic::object("text", "bye")("mostRecentEventCount", kTypedEventCount + 1), kTypedEventCount);

  CHECK(stateText(*input) == "bye");
  CHECK(input->getStateData().reactTreeAttributedString.getString() == "bye");
  CHECK(input->getStateData().mostRecentEventCount == kTypedEventCount + 1);
}

TEST_CASE_FIXTURE(Field, "a tree change that only resizes the field keeps the typed text, with no count to apply") {
  const auto input = typedInto(
      folly::dynamic::object("width", kFieldWidth / 2)("mostRecentEventCount", kTreeEventCount), kTreeEventCount);

  CHECK(stateText(*input) == "hello world");
  CHECK(input->getStateData().mostRecentEventCount == 0);
}

} // namespace rntizen
