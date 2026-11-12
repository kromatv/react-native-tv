#pragma once

#include "ComponentDescriptorRegistry.h"

#include <doctest/doctest.h>

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/componentregistry/ComponentDescriptorRegistry.h>
#include <react/renderer/components/view/YogaLayoutableShadowNode.h>
#include <react/renderer/core/ComponentDescriptor.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/core/LayoutContext.h>
#include <react/renderer/core/PropsParserContext.h>
#include <react/renderer/core/RawProps.h>
#include <react/renderer/core/ShadowNode.h>
#include <react/renderer/core/ShadowNodeFragment.h>
#include <react/renderer/textlayoutmanager/TextMeasurer.h>
#include <react/utils/ContextContainer.h>

#include <folly/dynamic.h>

#include <memory>
#include <string>
#include <utility>

namespace rntizen::testing {

class RecordingMeasurer final : public facebook::react::TextMeasurer {
 public:
  static constexpr facebook::react::Float kCharacterWidth = 10;
  static constexpr facebook::react::Float kLineHeight = 20;

  facebook::react::Size measure(
      const facebook::react::AttributedString &attributedString,
      const facebook::react::ParagraphAttributes &,
      facebook::react::Size maximumSize,
      facebook::react::Float) const override {
    measured = attributedString.getString();
    maximumWidth = maximumSize.width;
    return {.width = kCharacterWidth * static_cast<facebook::react::Float>(measured.size()), .height = kLineHeight};
  }

  mutable std::string measured;
  mutable facebook::react::Float maximumWidth{0};
};

class Components final {
 public:
  static constexpr facebook::react::SurfaceId kSurfaceId = 1;

  Components()
      : measurer_(std::make_shared<RecordingMeasurer>()),
        contextContainer_(withMeasurer(measurer_)),
        registry_(componentRegistryFactory()({}, contextContainer_)) {}

  const facebook::react::ComponentDescriptor &descriptor(const std::string &name) const {
    return registry_->at(name);
  }

  std::shared_ptr<facebook::react::ShadowNode> create(const std::string &name, folly::dynamic props) {
    const auto &componentDescriptor = descriptor(name);
    const facebook::react::PropsParserContext context(kSurfaceId, *contextContainer_);
    const auto parsed = componentDescriptor.cloneProps(context, nullptr, facebook::react::RawProps(std::move(props)));
    const auto family =
        componentDescriptor.createFamily({.tag = nextTag_++, .surfaceId = kSurfaceId, .instanceHandle = nullptr});
    const auto state = componentDescriptor.createInitialState(parsed, family);
    return componentDescriptor.createShadowNode({.props = parsed, .state = state}, family);
  }

  std::shared_ptr<facebook::react::ShadowNode> update(
      const facebook::react::ShadowNode &node,
      folly::dynamic props,
      const facebook::react::StateData::Shared &stateData) const {
    const auto &componentDescriptor = node.getComponentDescriptor();
    const facebook::react::PropsParserContext context(kSurfaceId, *contextContainer_);
    const auto parsed =
        componentDescriptor.cloneProps(context, node.getProps(), facebook::react::RawProps(std::move(props)));
    const auto state = stateData == nullptr ? facebook::react::ShadowNodeFragment::statePlaceholder()
                                            : componentDescriptor.createState(node.getFamily(), stateData);
    return componentDescriptor.cloneShadowNode(node, {.props = parsed, .state = state});
  }

  const RecordingMeasurer &measurer() const {
    return *measurer_;
  }

 private:
  static std::shared_ptr<const facebook::react::ContextContainer> withMeasurer(
      const std::shared_ptr<const RecordingMeasurer> &measurer) {
    auto context = std::make_shared<facebook::react::ContextContainer>();
    context->insert(
        facebook::react::TextMeasurer::contextKey, std::shared_ptr<const facebook::react::TextMeasurer>(measurer));
    return context;
  }

  std::shared_ptr<RecordingMeasurer> measurer_;
  std::shared_ptr<const facebook::react::ContextContainer> contextContainer_;
  facebook::react::ComponentDescriptorRegistry::Shared registry_;
  facebook::react::Tag nextTag_{1};
};

inline constexpr facebook::react::LayoutConstraints kRoom{
    .minimumSize = {.width = 0, .height = 0},
    .maximumSize = {.width = 200, .height = 100}};

inline facebook::react::Size laidOutSize(
    std::shared_ptr<facebook::react::ShadowNode> node,
    const facebook::react::LayoutConstraints &constraints = kRoom) {
  const auto layoutable = std::dynamic_pointer_cast<facebook::react::YogaLayoutableShadowNode>(std::move(node));
  REQUIRE(layoutable != nullptr);
  layoutable->layoutTree(facebook::react::LayoutContext{}, constraints);
  return layoutable->getLayoutMetrics().frame.size;
}

} // namespace rntizen::testing
