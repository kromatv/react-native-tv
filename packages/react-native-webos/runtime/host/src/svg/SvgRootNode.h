#pragma once

#include "SvgGroupNode.h"
#include "SvgViewBox.h"

#include <functional>

namespace rnwebos::svg {

/// The root of the nodes one `<Svg>` draws: its viewBox, the color
/// `currentColor` stands for (`color`), and who to tell when the tree changes.
class RootNode final : public GroupNode {
 public:
  SvgViewBox viewBox;
  std::function<void()> onChange;

  /// Draws the tree at the canvas's origin into a box of `size` layout
  /// points, `scale` pixels to the point. User units are points; a viewBox
  /// maps its own units into the box. `alpha` is an opacity the tree's one
  /// paint applies: pass it only when `paintLayers` is 1 for the same size.
  void renderInto(SkCanvas &canvas, SkSize size, float scale, float alpha = 1) const;
  /// How many paints the tree lays over each other: 0, 1, or 2 for more.
  int paintLayers(SkSize size) const;

 protected:
  void changed() override;

 private:
  RenderContext rootContext(SkSize size, const Definitions &definitions) const;
};

} // namespace rnwebos::svg
