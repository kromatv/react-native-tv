#pragma once

#include "ComponentView.h"

#include <react/renderer/components/text/ParagraphShadowNode.h>

#include <memory>
#include <optional>
#include <string>

namespace rntizen {

/// A `<Text>`: an Evas textblock filled from the paragraph's attributed string
/// at the surface's scale, laid out inside the view's padding.
class ParagraphView final : public ComponentView {
 public:
  ParagraphView(Evas *evas, facebook::react::Tag tag);
  ~ParagraphView() override;

  void updateState(const facebook::react::State::Shared &state) override;
  void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) override;

 protected:
  void layoutMembers() override;

 private:
  void applyContent();

  Evas_Object *textblock_;
  Evas_Textblock_Style *style_;
  std::shared_ptr<const facebook::react::ParagraphShadowNode::ConcreteState> paragraph_;
  std::string markup_;
  std::optional<int> tightLineShift_;
  Evas_Coord firstLineTop_{-1};
  facebook::react::Float contentScale_{0};
  Evas_Coord clampedWidth_{-1};
  bool clamped_{false};
};

} // namespace rntizen
