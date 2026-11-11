#pragma once

#include "../text/ParagraphLayout.h"
#include "ComponentView.h"

#include <react/renderer/components/text/ParagraphShadowNode.h>

#include <memory>

namespace rnwebos {

/// A `<Text>`: the paragraph's attributed string laid out by SkParagraph at
/// the surface's scale, inside the view's padding. The layout is made when the
/// view is first drawn, so text that never comes on screen costs nothing.
class ParagraphView final : public ComponentView {
 public:
  using ComponentView::ComponentView;

  void updateState(const facebook::react::State::Shared &state) override;

 protected:
  void drawContent(SkCanvas &canvas, float alpha) override;
  int contentLayers() const override;
  void layoutContent() override;

 private:
  PixelRect contentRect() const;

  std::shared_ptr<const facebook::react::ParagraphShadowNode::ConcreteState> paragraph_;
  ParagraphLayout layout_;
  PixelRect laidOutIn_;
  facebook::react::Float laidOutScale_{0};
};

} // namespace rnwebos
