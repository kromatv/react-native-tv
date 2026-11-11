#include "SvgGroupNode.h"
#include "SvgRootNode.h"
#include "SvgShapeNode.h"

#include "include/core/SkBitmap.h"

#include <doctest/doctest.h>

#include <memory>

namespace rnwebos::svg {

namespace {

struct CountedRoot {
  RootNode root;
  int changes{0};

  CountedRoot() {
    root.onChange = [this] { ++changes; };
  }
};

/// A shape that shows the path it was asked to draw.
class ProbeNode final : public ShapeNode {
 public:
  using ShapeNode::forgetPath;

  mutable int built{0};

 protected:
  SkPath buildPath(SkSize) const override {
    ++built;
    return SkPath::Rect(SkRect::MakeWH(10, 10));
  }
};

} // namespace

TEST_CASE("a change anywhere in the tree reaches the root") {
  CountedRoot tree;
  GroupNode group;
  RectNode rect;
  tree.root.setChildren({&group});
  group.setChildren({&rect});
  tree.changes = 0;

  rect.invalidate();

  CHECK(tree.changes == 1);
}

TEST_CASE("mounting and unmounting children tells the root") {
  CountedRoot tree;
  GroupNode group;
  tree.root.setChildren({&group});
  RectNode rect;
  tree.changes = 0;

  group.setChildren({&rect});
  CHECK(tree.changes == 1);
  CHECK(rect.parent() == &group);

  group.setChildren({});
  CHECK(tree.changes == 2);
  CHECK(rect.parent() == nullptr);
}

TEST_CASE("a node that goes leaves its parent and tells the root") {
  CountedRoot tree;
  auto rect = std::make_unique<RectNode>();
  tree.root.setChildren({rect.get()});
  tree.changes = 0;

  rect.reset();

  CHECK(tree.root.children().empty());
  CHECK(tree.changes == 1);
}

TEST_CASE("a parent that goes first leaves its children without a parent") {
  RectNode rect;
  {
    GroupNode group;
    group.setChildren({&rect});
  }

  CHECK(rect.parent() == nullptr);
  rect.invalidate();
}

TEST_CASE("a node moved to another parent leaves the first") {
  GroupNode first;
  GroupNode second;
  RectNode rect;
  first.setChildren({&rect});

  second.setChildren({&rect});

  CHECK(first.children().empty());
  CHECK(rect.parent() == &second);
}

TEST_CASE("a shape builds its path once, until its geometry is forgotten") {
  RootNode root;
  ProbeNode probe;
  root.setChildren({&probe});
  SkBitmap pixels;
  pixels.allocN32Pixels(20, 20);
  SkCanvas canvas(pixels);

  root.renderInto(canvas, {20, 20}, 1);
  probe.paint.fillOpacity = 0.5F;
  probe.invalidate();
  root.renderInto(canvas, {20, 20}, 1);
  CHECK(probe.built == 1);

  probe.forgetPath();
  probe.invalidate();
  root.renderInto(canvas, {20, 20}, 1);
  CHECK(probe.built == 2);
}

TEST_CASE("a shape builds its path again when percentages resolve against another size") {
  RootNode root;
  ProbeNode probe;
  root.setChildren({&probe});
  SkBitmap pixels;
  pixels.allocN32Pixels(20, 20);
  SkCanvas canvas(pixels);

  root.renderInto(canvas, {20, 20}, 1);
  root.renderInto(canvas, {20, 20}, 2);
  CHECK(probe.built == 1);

  root.renderInto(canvas, {30, 20}, 1);
  CHECK(probe.built == 2);
}

TEST_CASE("setting the same geometry again keeps the built path") {
  RectNode rect;
  const RectGeometry geometry{.width = {.value = 10}, .height = {.value = 10}};
  rect.setGeometry(geometry);
  RootNode root;
  root.setChildren({&rect});
  const Definitions definitions(root);
  const RenderContext context{.viewport = {20, 20}, .definitions = &definitions};
  const auto before = rect.bounds(context);

  rect.setGeometry(geometry);

  CHECK(rect.bounds(context) == before);
  CHECK(before == SkRect::MakeWH(10, 10));
}

} // namespace rnwebos::svg
