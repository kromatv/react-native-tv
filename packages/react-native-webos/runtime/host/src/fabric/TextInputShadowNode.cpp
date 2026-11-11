#include "TextInputShadowNode.h"

#include <react/renderer/attributedstring/AttributedStringBox.h>
#include <react/renderer/components/text/BaseTextShadowNode.h>
#include <react/renderer/core/LayoutContext.h>
#include <react/renderer/textlayoutmanager/TextLayoutContext.h>

#include <limits>

namespace rntizen {

using namespace facebook::react;

extern const char TextInputComponentName[] = "AndroidTextInput";

namespace {

// An empty string has no run to carry the input's font, so its one line would
// measure in the textblock's default size; a zero-width space carries it.
constexpr const char *kEmptyLine = "​";

} // namespace

ShadowNodeTraits TextInputShadowNode::BaseTraits() {
  auto traits = ConcreteViewShadowNode::BaseTraits();
  traits.set(ShadowNodeTraits::Trait::LeafYogaNode);
  traits.set(ShadowNodeTraits::Trait::MeasurableYogaNode);
  return traits;
}

void TextInputShadowNode::setTextLayoutManager(std::shared_ptr<const TextLayoutManager> textLayoutManager) {
  ensureUnsealed();
  textLayoutManager_ = std::move(textLayoutManager);
}

Size TextInputShadowNode::measureContent(
    const LayoutContext &layoutContext,
    const LayoutConstraints &layoutConstraints) const {
  auto textConstraints = layoutConstraints;
  if (!getConcreteProps().multiline) textConstraints.maximumSize.width = std::numeric_limits<Float>::infinity();
  auto attributedString = mostRecentAttributedString(layoutContext);
  if (attributedString.isEmpty()) attributedString = placeholderAttributedString(layoutContext);
  const TextLayoutContext textLayoutContext{.pointScaleFactor = layoutContext.pointScaleFactor, .surfaceId = getSurfaceId()};
  auto size = textLayoutManager_
                  ->measure(
                      AttributedStringBox{attributedString},
                      getConcreteProps().paragraphAttributes,
                      textLayoutContext,
                      textConstraints)
                  .size;
  if (attributedString.getString() == kEmptyLine) size.width = 0;
  return layoutConstraints.clamp(size);
}

void TextInputShadowNode::layout(LayoutContext layoutContext) {
  updateStateIfNeeded(layoutContext);
  ConcreteViewShadowNode::layout(layoutContext);
}

void TextInputShadowNode::updateStateIfNeeded(const LayoutContext &layoutContext) {
  ensureUnsealed();
  const auto &stateData = getStateData();
  auto reactTreeAttributedString = treeAttributedString(layoutContext);
  if (stateData.reactTreeAttributedString == reactTreeAttributedString) return;
  const auto &props = getConcreteProps();
  if (props.mostRecentEventCount < stateData.mostRecentEventCount) return;
  const auto eventCount =
      stateData.reactTreeAttributedString.isContentEqual(reactTreeAttributedString) ? 0 : props.mostRecentEventCount;
  setStateData(TextInputState{
      AttributedStringBox(mostRecentAttributedString(layoutContext)),
      reactTreeAttributedString,
      props.paragraphAttributes,
      eventCount});
}

AttributedString TextInputShadowNode::treeAttributedString(const LayoutContext &layoutContext) const {
  const auto &props = getConcreteProps();
  auto childTextAttributes = TextAttributes::defaultTextAttributes();
  childTextAttributes.fontSizeMultiplier = layoutContext.fontSizeMultiplier;
  childTextAttributes.apply(props.textAttributes);
  childTextAttributes.backgroundColor = HostPlatformColor::UndefinedColor;

  AttributedString attributedString;
  BaseTextShadowNode::Attachments attachments;
  BaseTextShadowNode::buildAttributedString(childTextAttributes, *this, attributedString, attachments);
  attributedString.setBaseTextAttributes(childTextAttributes);

  if (!props.text.empty()) {
    auto textAttributes = TextAttributes::defaultTextAttributes();
    textAttributes.apply(props.textAttributes);
    textAttributes.fontSizeMultiplier = layoutContext.fontSizeMultiplier;
    AttributedString::Fragment fragment;
    fragment.string = props.text;
    fragment.textAttributes = textAttributes;
    fragment.textAttributes.backgroundColor = clearColor();
    fragment.parentShadowView = ShadowView(*this);
    attributedString.prependFragment(std::move(fragment));
  }
  return attributedString;
}

AttributedString TextInputShadowNode::mostRecentAttributedString(const LayoutContext &layoutContext) const {
  const auto &state = getStateData();
  auto reactTreeAttributedString = treeAttributedString(layoutContext);
  const bool treeChanged = !state.reactTreeAttributedString.compareTextAttributesWithoutFrame(reactTreeAttributedString);
  return treeChanged ? reactTreeAttributedString : state.attributedStringBox.getValue();
}

AttributedString TextInputShadowNode::placeholderAttributedString(const LayoutContext &layoutContext) const {
  const auto &props = getConcreteProps();
  AttributedString attributedString;
  attributedString.setBaseTextAttributes(props.getEffectiveTextAttributes(layoutContext.fontSizeMultiplier));
  attributedString.appendFragment(
      {.string = props.placeholder.empty() ? std::string(kEmptyLine) : props.placeholder,
       .textAttributes = attributedString.getBaseTextAttributes(),
       .parentShadowView = {}});
  return attributedString;
}

} // namespace rntizen
