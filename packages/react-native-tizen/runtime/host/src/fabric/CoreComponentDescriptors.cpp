#include "../mounting/LogicalScreen.h"
#include "ComponentDescriptorRegistry.h"

#include <react/renderer/components/FBReactNativeSpec/ComponentDescriptors.h>
#include <react/renderer/components/image/ImageComponentDescriptor.h>
#include <react/renderer/components/modal/ModalHostViewShadowNode.h>
#include <react/renderer/components/root/RootComponentDescriptor.h>
#include <react/renderer/components/scrollview/ScrollViewComponentDescriptor.h>
#include <react/renderer/components/text/ParagraphComponentDescriptor.h>
#include <react/renderer/components/text/RawTextComponentDescriptor.h>
#include <react/renderer/components/text/TextComponentDescriptor.h>
#include <react/renderer/components/view/ConcreteViewShadowNode.h>
#include <react/renderer/components/view/ViewComponentDescriptor.h>

namespace rntizen {

namespace {

/// A Modal's descriptor: React Native's, sized to the logical screen from the
/// first layout, where the portable one starts at 0x0 and waits for a native
/// state update.
class ModalHostViewComponentDescriptor final
    : public facebook::react::ConcreteComponentDescriptor<facebook::react::ModalHostViewShadowNode> {
 public:
  using ConcreteComponentDescriptor::ConcreteComponentDescriptor;

  void adopt(facebook::react::ShadowNode &shadowNode) const override {
    auto &layoutable = static_cast<facebook::react::YogaLayoutableShadowNode &>(shadowNode);
    const auto &state = static_cast<const ConcreteState &>(*shadowNode.getState()).getData();
    layoutable.setSize(state.screenSize);
    layoutable.setPositionType(YGPositionTypeAbsolute);
    ConcreteComponentDescriptor::adopt(shadowNode);
  }

  facebook::react::State::Shared createInitialState(
      const facebook::react::Props::Shared &,
      const facebook::react::ShadowNodeFamily::Shared &family) const override {
    return std::make_shared<ConcreteState>(std::make_shared<const ConcreteStateData>(kLogicalScreenSize), family);
  }
};

inline constexpr char kMaskedViewComponentName[] = "RNCMaskedView";

/// @react-native-masked-view/masked-view's view, which has no codegen spec: a
/// plain view's props and layout under its own name (`views/MaskedView.cpp`).
using MaskedViewComponentDescriptor = facebook::react::ConcreteComponentDescriptor<
    facebook::react::ConcreteViewShadowNode<kMaskedViewComponentName, facebook::react::ViewProps>>;

} // namespace

} // namespace rntizen

RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::RootComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::ViewComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::TextComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::RawTextComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::ParagraphComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::ImageComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::ScrollViewComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(rntizen::ModalHostViewComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(rntizen::MaskedViewComponentDescriptor);
RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::ActivityIndicatorViewComponentDescriptor);
