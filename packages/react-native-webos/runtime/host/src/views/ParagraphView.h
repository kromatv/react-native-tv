#pragma once

#include "../text/ParagraphLayout.h"
#include "../text/PreparedParagraph.h"
#include "ComponentView.h"

#include <react/renderer/components/text/ParagraphShadowNode.h>

#include <memory>

namespace rnwebos {

/// A `<Text>`, inside the view's padding: the paragraph its shadow node
/// prepared, played as recorded. A state without one has its attributed string
/// laid out by SkParagraph at the surface's scale when the view is first drawn.
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
  std::shared_ptr<const PreparedParagraph> prepared_;
  ParagraphLayout layout_;
  PixelRect laidOutIn_;
  facebook::react::Float laidOutScale_{0};
};

} // namespace rnwebos
