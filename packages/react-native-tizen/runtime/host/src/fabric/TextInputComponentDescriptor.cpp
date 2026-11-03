#include "ComponentDescriptorRegistry.h"
#include "TextInputShadowNode.h"

#include <react/renderer/core/ConcreteComponentDescriptor.h>

namespace rntizen {

namespace {

/// `AndroidTextInput`'s descriptor: Android's without the theme padding it
/// reads from Java, so an input with no padding has none, as on iOS.
class TextInputComponentDescriptor final : public facebook::react::ConcreteComponentDescriptor<TextInputShadowNode> {
 public:
  explicit TextInputComponentDescriptor(const facebook::react::ComponentDescriptorParameters &parameters)
      : ConcreteComponentDescriptor(parameters),
        textLayoutManager_(std::make_shared<facebook::react::TextLayoutManager>(contextContainer_)) {}

 protected:
  void adopt(facebook::react::ShadowNode &shadowNode) const override {
    auto &textInput = static_cast<TextInputShadowNode &>(shadowNode);
    textInput.setTextLayoutManager(textLayoutManager_);
    textInput.dirtyLayout();
    ConcreteComponentDescriptor::adopt(shadowNode);
  }

 private:
  const std::shared_ptr<facebook::react::TextLayoutManager> textLayoutManager_;
};

} // namespace

} // namespace rntizen

RN_TIZEN_COMPONENT_DESCRIPTOR(rntizen::TextInputComponentDescriptor);
