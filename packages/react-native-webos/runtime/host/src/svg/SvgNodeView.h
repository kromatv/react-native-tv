#pragma once

#include "../views/ComponentView.h"
#include "SvgNode.h"

#include <span>
#include <vector>

namespace rnwebos::svg {

/// A react-native-svg element mounted inside an `<Svg>`: the view that keeps
/// one node of the tree the `<Svg>` draws in step with Fabric. It draws
/// nothing itself.
class SvgNodeView : public ComponentView {
 public:
  using ComponentView::ComponentView;

  virtual Node &node() = 0;

  void draw(SkCanvas &canvas) override;

 protected:
  void didChangeChildren() override;
};

/// The nodes of `views`, in order. A view that is no node (an element with no
/// implementation here) is left out with its subtree.
std::vector<Node *> nodesOf(std::span<ComponentView *const> views);

/// The view of the elements whose props `read` turns into a `NodeType`.
template <typename NodeType, typename Props, void (*read)(NodeType &, const Props &)>
class SvgNodeViewOf final : public SvgNodeView {
 public:
  using SvgNodeView::SvgNodeView;

  Node &node() override {
    return node_;
  }

  void updateProps(const facebook::react::Props::Shared &props) override {
    ComponentView::updateProps(props);
    read(node_, static_cast<const Props &>(*props));
    node_.invalidate();
  }

 private:
  NodeType node_;
};

} // namespace rnwebos::svg
