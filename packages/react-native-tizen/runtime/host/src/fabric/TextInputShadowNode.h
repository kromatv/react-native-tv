#pragma once

#include "TextInputProps.h"

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/components/textinput/TextInputState.h>
#include <react/renderer/components/view/ConcreteViewShadowNode.h>
#include <react/renderer/components/view/ViewEventEmitter.h>
#include <react/renderer/textlayoutmanager/TextLayoutManager.h>

#include <memory>

namespace rntizen {

extern const char TextInputComponentName[];

/// React Native's `AndroidTextInput` on Tizen, with
/// `AndroidTextInputShadowNode`'s layout and state rules, measured by the
/// host's text measurer. ReactCommon's own node cannot build here: it asks the
/// TextLayoutManager for Java's cached spannables and for line measurements,
/// which Tizen's does not have. Its events go out with Android's names and
/// payloads, straight through the view's event emitter, as Java sends them.
class TextInputShadowNode final : public facebook::react::ConcreteViewShadowNode<
                                      TextInputComponentName,
                                      TextInputProps,
                                      facebook::react::ViewEventEmitter,
                                      facebook::react::TextInputState> {
 public:
  using ConcreteViewShadowNode::ConcreteViewShadowNode;

  static facebook::react::ShadowNodeTraits BaseTraits();

  void setTextLayoutManager(std::shared_ptr<const facebook::react::TextLayoutManager> textLayoutManager);

 protected:
  facebook::react::Size measureContent(
      const facebook::react::LayoutContext &layoutContext,
      const facebook::react::LayoutConstraints &layoutConstraints) const override;

  void layout(facebook::react::LayoutContext layoutContext) override;

 private:
  void updateStateIfNeeded(const facebook::react::LayoutContext &layoutContext);
  facebook::react::AttributedString treeAttributedString(const facebook::react::LayoutContext &layoutContext) const;
  facebook::react::AttributedString mostRecentAttributedString(const facebook::react::LayoutContext &layoutContext) const;
  facebook::react::AttributedString placeholderAttributedString(const facebook::react::LayoutContext &layoutContext) const;

  std::shared_ptr<const facebook::react::TextLayoutManager> textLayoutManager_;
};

} // namespace rntizen
