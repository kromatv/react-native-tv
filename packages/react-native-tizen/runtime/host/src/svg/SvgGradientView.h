#pragma once

#include "SvgGradient.h"
#include "SvgNodeView.h"

#include <string>
#include <vector>

namespace rntizen::svg {

/// `<LinearGradient>` or `<RadialGradient>`: draws nothing in place, and is
/// found by its `id` when a shape paints with `url(#id)`. A change to it paints
/// its `<Svg>` again, as any node's does.
class SvgGradientView : public SvgNodeView {
 public:
  using SvgNodeView::SvgNodeView;

  const std::string &name() const;
  const GradientDefinition &definition() const;

 protected:
  void drawContent(Drawing &drawing, const DrawContext &context) const final;

  std::string name_;
  GradientDefinition definition_;
};

/// Adds every gradient in `views` and their subtrees to `gradients`; the first
/// one with an `id` wins, as `getElementById` picks.
void collectGradients(const std::vector<ComponentView *> &views, GradientDefinitions &gradients);

} // namespace rntizen::svg
