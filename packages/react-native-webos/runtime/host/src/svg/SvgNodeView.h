#pragma once

#include "../views/ComponentView.h"
#include "SvgNode.h"

#include <memory>
#include <span>

namespace rntizen::svg {

class SvgView;

/// A react-native-svg element mounted inside an `<Svg>`. It paints nothing
/// itself: the `<Svg>` walks its nodes into one Evas VG object, and a node asks
/// it to paint again when its props change, when it is mounted, unmounted or
/// destroyed, so the `<Svg>` a node leaves paints without it.
class SvgNodeView : public ComponentView {
 public:
  SvgNodeView(Evas *evas, facebook::react::Tag tag);
  ~SvgNodeView() override;

  void updateProps(const facebook::react::Props::Shared &props) override;

  /// Appends what this node and its subtree paint, unless it is `display="none"`.
  void draw(Drawing &drawing, const DrawContext &parent) const;

 protected:
  void didMount() override;
  void willUnmount() override;
  virtual void readProps(const facebook::react::Props &props) = 0;
  virtual void drawContent(Drawing &drawing, const DrawContext &context) const = 0;

  template <typename Props>
  void readRenderable(const Props &props) {
    node_ = readNodeAttributes(props);
    paint_ = readPaint(props);
  }

 private:
  void requestRedraw() noexcept;

  NodeAttributes node_;
  Paint paint_;
  std::weak_ptr<SvgView *const> owner_;
};

/// Paints the nodes among `children` in order. A child that is no node (`<Defs>`,
/// a gradient, anything not drawn in place) is skipped with its subtree.
void drawChildren(std::span<ComponentView *const> children, Drawing &drawing, const DrawContext &context);

} // namespace rntizen::svg
