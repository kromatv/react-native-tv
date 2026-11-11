#include <react/renderer/textlayoutmanager/TextLayoutManager.h>

#include <react/renderer/textlayoutmanager/TextMeasurer.h>

#include <doctest/doctest.h>

#include <memory>

namespace facebook::react {

namespace {

class CountingMeasurer final : public TextMeasurer {
 public:
  Size measure(const AttributedString &, const ParagraphAttributes &, Size, Float) const override {
    ++calls;
    return {.width = 300, .height = 40};
  }

  mutable int calls{0};
};

AttributedString paragraphWithAttachment() {
  AttributedString string;
  string.appendFragment({.string = "Hello", .textAttributes = {}, .parentShadowView = {}});
  string.appendFragment(
      {.string = AttributedString::Fragment::AttachmentCharacter(), .textAttributes = {}, .parentShadowView = {}});
  return string;
}

} // namespace

TEST_CASE("TextLayoutManager measures through the host's measurer once, clamps the size and lists attachments") {
  const auto measurer = std::make_shared<CountingMeasurer>();
  const auto context = std::make_shared<ContextContainer>();
  context->insert(TextMeasurer::contextKey, std::shared_ptr<const TextMeasurer>(measurer));
  const TextLayoutManager manager(context);
  const AttributedStringBox text(paragraphWithAttachment());
  LayoutConstraints constraints{};
  constraints.maximumSize = {.width = 200, .height = 100};

  const auto first = manager.measure(text, {}, {}, constraints);
  const auto second = manager.measure(text, {}, {}, constraints);

  CHECK(measurer->calls == 1);
  CHECK(first.size == Size{.width = 200, .height = 40});
  CHECK(second.size == first.size);
  REQUIRE(first.attachments.size() == 1);
  CHECK(first.attachments[0].isClipped);
}

} // namespace facebook::react
