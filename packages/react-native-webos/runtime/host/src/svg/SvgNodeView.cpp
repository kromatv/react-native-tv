#include "SvgNodeView.h"

namespace rnwebos::svg {

void SvgNodeView::draw(SkCanvas &) {
  // The `<Svg>` draws its nodes: a node's own box, as a view, shows nothing.
}

void SvgNodeView::didChangeChildren() {
  node().setChildren(nodesOf(children()));
}

std::vector<Node *> nodesOf(std::span<ComponentView *const> views) {
  std::vector<Node *> nodes;
  nodes.reserve(views.size());
  for (auto *view : views) {
    if (auto *nodeView = dynamic_cast<SvgNodeView *>(view)) nodes.push_back(&nodeView->node());
  }
  return nodes;
}

} // namespace rnwebos::svg
