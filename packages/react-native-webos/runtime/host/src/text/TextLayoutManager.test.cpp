#include <react/renderer/textlayoutmanager/TextLayoutManager.h>

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

  mutable int calls{0};
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

} // namespace facebook::react
