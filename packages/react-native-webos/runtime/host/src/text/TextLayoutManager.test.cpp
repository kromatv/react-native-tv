#include <react/renderer/textlayoutmanager/TextLayoutManager.h>

#include <react/renderer/textlayoutmanager/TextLayoutManagerExtended.h>
#include <react/renderer/textlayoutmanager/TextMeasurer.h>

#include <doctest/doctest.h>

#include <memory>

namespace facebook::react {

namespace {

const Rect kAttachmentFrame{.origin = {.x = 50, .y = 4}, .size = {.width = 30, .height = 20}};

class CountingMeasurer final : public TextMeasurer {
 public:
  TextMeasurement measure(const AttributedString &, const ParagraphAttributes &, Size maximumSize, Float pointScaleFactor)
      const override {
    ++calls;
    maximumWidth = maximumSize.width;
    scale = pointScaleFactor;
    TextMeasurement measurement{};
    measurement.size = {.width = 300, .height = 40};
    measurement.attachments.push_back({.frame = kAttachmentFrame, .isClipped = false});
    return measurement;
  }

  LinesMeasurements measureLines(const AttributedString &, const ParagraphAttributes &, Size size) const override {
    ++lineCalls;
    return {LineMeasurement("Hello", {.origin = {}, .size = {.width = size.width, .height = 24}}, 5, 14, 19, 10)};
  }

  mutable int calls{0};
  mutable int lineCalls{0};
  mutable Float maximumWidth{0};
  mutable Float scale{0};
};

AttributedString paragraphWithAttachment() {
  AttributedString string;
  string.appendFragment({.string = "Hello", .textAttributes = {}, .parentShadowView = {}});
  string.appendFragment(
      {.string = AttributedString::Fragment::AttachmentCharacter(), .textAttributes = {}, .parentShadowView = {}});
  return string;
}

} // namespace

TEST_CASE("TextLayoutManager measures through the host's measurer once, clamps the size and keeps its attachments") {
  const auto measurer = std::make_shared<CountingMeasurer>();
  const auto context = std::make_shared<ContextContainer>();
  context->insert(TextMeasurer::contextKey, std::shared_ptr<const TextMeasurer>(measurer));
  const TextLayoutManager manager(context);
  const AttributedStringBox text(paragraphWithAttachment());
  LayoutConstraints constraints{};
  constraints.maximumSize = {.width = 200, .height = 100};
  const TextLayoutContext layoutContext{.pointScaleFactor = 2};

  const auto first = manager.measure(text, {}, layoutContext, constraints);
  const auto second = manager.measure(text, {}, layoutContext, constraints);

  CHECK(measurer->calls == 1);
  CHECK(measurer->maximumWidth == 200);
  CHECK(measurer->scale == 2);
  CHECK(first.size == Size{.width = 200, .height = 40});
  CHECK(second.size == first.size);
  REQUIRE(first.attachments.size() == 1);
  CHECK(first.attachments[0].frame == kAttachmentFrame);
  CHECK_FALSE(first.attachments[0].isClipped);
}

TEST_CASE("TextLayoutManager measures again for another width or another scale") {
  const auto measurer = std::make_shared<CountingMeasurer>();
  const auto context = std::make_shared<ContextContainer>();
  context->insert(TextMeasurer::contextKey, std::shared_ptr<const TextMeasurer>(measurer));
  const TextLayoutManager manager(context);
  const AttributedStringBox text(paragraphWithAttachment());
  LayoutConstraints wide{};
  wide.maximumSize = {.width = 400, .height = 100};
  LayoutConstraints narrow{};
  narrow.maximumSize = {.width = 200, .height = 100};

  manager.measure(text, {}, {.pointScaleFactor = 1}, wide);
  manager.measure(text, {}, {.pointScaleFactor = 1}, narrow);
  manager.measure(text, {}, {.pointScaleFactor = 2}, narrow);

  CHECK(measurer->calls == 3);
}

TEST_CASE("TextLayoutManager prepares a paragraph once for the same text, box and scale, and clamps its size") {
  const auto measurer = std::make_shared<CountingMeasurer>();
  const auto context = std::make_shared<ContextContainer>();
  context->insert(TextMeasurer::contextKey, std::shared_ptr<const TextMeasurer>(measurer));
  const TextLayoutManager manager(context);
  const auto text = paragraphWithAttachment();
  LayoutConstraints constraints{};
  constraints.maximumSize = {.width = 200, .height = 100};
  const TextLayoutContext layoutContext{.pointScaleFactor = 2};

  const auto first = manager.prepareLayout(text, {}, layoutContext, constraints);
  const auto second = manager.prepareLayout(text, {}, layoutContext, constraints);
  const auto atAnotherScale = manager.prepareLayout(text, {}, {.pointScaleFactor = 1}, constraints);
  const auto measurement = manager.measurePreparedLayout(first, layoutContext, constraints);

  REQUIRE(first != nullptr);
  CHECK(second == first);
  CHECK(atAnotherScale != first);
  CHECK(measurer->calls == 2);
  CHECK(first->measurement.size == Size{.width = 300, .height = 40});
  CHECK(measurement.size == Size{.width = 200, .height = 40});
  REQUIRE(measurement.attachments.size() == 1);
  CHECK(measurement.attachments[0].frame == kAttachmentFrame);
}

TEST_CASE("TextLayoutManager measures a paragraph's lines once for a size, for onTextLayout and the baseline") {
  const auto measurer = std::make_shared<CountingMeasurer>();
  const auto context = std::make_shared<ContextContainer>();
  context->insert(TextMeasurer::contextKey, std::shared_ptr<const TextMeasurer>(measurer));
  const TextLayoutManager manager(context);
  const AttributedStringBox text(paragraphWithAttachment());

  const auto lines = manager.measureLines(text, {}, {.width = 200, .height = 100});
  manager.measureLines(text, {}, {.width = 200, .height = 100});
  manager.measureLines(text, {}, {.width = 300, .height = 100});

  CHECK(TextLayoutManagerExtended::supportsLineMeasurement());
  CHECK(measurer->lineCalls == 2);
  REQUIRE(lines.size() == 1);
  CHECK(LineMeasurement::baseline(lines) == 19);
  CHECK(lines[0].frame.size.width == 200);
}

TEST_CASE("TextLayoutManager prepares text again when only its color differs, which measures the same") {
  const auto measurer = std::make_shared<CountingMeasurer>();
  const auto context = std::make_shared<ContextContainer>();
  context->insert(TextMeasurer::contextKey, std::shared_ptr<const TextMeasurer>(measurer));
  const TextLayoutManager manager(context);
  const auto colored = [](SharedColor color) {
    TextAttributes attributes;
    attributes.foregroundColor = color;
    AttributedString string;
    string.appendFragment({.string = "Hello", .textAttributes = attributes, .parentShadowView = {}});
    return string;
  };
  const auto white = colored(colorFromRGBA(255, 255, 255, 255));
  const auto red = colored(colorFromRGBA(255, 0, 0, 255));
  LayoutConstraints constraints{};
  constraints.maximumSize = {.width = 200, .height = 100};
  const TextLayoutContext layoutContext{.pointScaleFactor = 1};

  manager.measure(AttributedStringBox(white), {}, layoutContext, constraints);
  manager.measure(AttributedStringBox(red), {}, layoutContext, constraints);
  const auto preparedWhite = manager.prepareLayout(white, {}, layoutContext, constraints);
  const auto preparedRed = manager.prepareLayout(red, {}, layoutContext, constraints);

  CHECK(measurer->calls == 3);
  CHECK(preparedRed != preparedWhite);
}

} // namespace facebook::react
