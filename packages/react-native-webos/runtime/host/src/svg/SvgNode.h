#pragma once

#include "SvgFont.h"
#include "SvgPaint.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPath.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkSize.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace rnwebos::svg {

class Node;

/// The named nodes of one `<Svg>`: what `url(#id)` and `href` resolve to. The
/// first node with a name wins, as `getElementById` picks.
class Definitions {
 public:
  explicit Definitions(const Node &root);

  const Node *find(std::string_view name) const;

  template <typename NodeType>
  const NodeType *findAs(std::string_view name) const {
    return dynamic_cast<const NodeType *>(find(name));
  }

 private:
  void collect(const Node &node);

  std::unordered_map<std::string_view, const Node *> nodes_;
};

/// What a percentage that is neither a width nor a height resolves against:
/// SVG's normalized diagonal of the viewport.
float diagonalOf(SkSize viewport);

/// Where the next glyphs of a `<Text>` go.
struct TextCursor {
  SkPoint pen;
};

/// What a node draws with: the paint, color and font it inherits, and what the
/// `<Svg>` gives every node.
struct RenderContext {
  Paint paint;
  SkColor4f currentColor{0, 0, 0, 1};
  Font font;
  /// Set inside a `<Text>`, whose spans share one pen.
  TextCursor *text{nullptr};
  /// The size percentages resolve against: the viewBox's, else the view's.
  SkSize viewport;
  /// An opacity the node's one paint applies itself, which spares a layer.
  float alpha{1};
  const Definitions *definitions{nullptr};
  /// How many references deep the drawing is, which ends a cycle of them.
  int depth{0};

  float viewportDiagonal() const;
  /// Whether a node may follow one more reference from here.
  bool canFollowReference() const;
};

/// One react-native-svg element, with what every element carries. Nodes form
/// the tree an `<Svg>` draws; a node neither owns its children nor its parent.
class Node {
 public:
  Node() = default;
  virtual ~Node();

  Node(const Node &) = delete;
  Node &operator=(const Node &) = delete;

  std::string name;
  SkMatrix matrix;
  float opacity{1};
  bool hidden{false};
  std::optional<SkColor4f> color;
  std::string clipPath;
  int clipRule{0};
  std::string mask;
  Paint paint;

  Node *parent() const;
  std::span<Node *const> children() const;
  /// Replaces the children, in drawing order. A node has one parent at most.
  void setChildren(std::vector<Node *> children);
  /// Reports to the root of the tree that the subtree draws something else now.
  void invalidate();

  /// Whether the node is drawn only through a reference to it, never in place.
  virtual bool definesOnly() const;
  /// Draws the subtree where `parent` draws its children, unless it is hidden.
  void render(SkCanvas &canvas, const RenderContext &parent) const;
  /// How many paints the subtree lays over each other in `parent`: 0, 1, or 2
  /// for more.
  int paintCount(const RenderContext &parent) const;
  /// The box around what the subtree fills, in the user space its content is
  /// drawn in. Empty when it fills nothing.
  SkRect bounds(const RenderContext &parent) const;
  /// Adds what the subtree fills, through `transform`, to `outlines`: the area
  /// a `<ClipPath>` holding the node keeps.
  void appendOutlines(std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &parent) const;

 protected:
  virtual void draw(SkCanvas &canvas, const RenderContext &context) const;
  virtual int contentPaintCount(const RenderContext &context) const;
  virtual SkRect contentBounds(const RenderContext &context) const;
  virtual void appendContentOutlines(
      std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &context) const;
  /// Adds what the node hands down besides paint and color.
  virtual void inherit(RenderContext &context) const;
  /// Runs on the root of a tree when something in it changed.
  virtual void changed();

 private:
  RenderContext inherited(const RenderContext &parent) const;
  bool skipped() const;

  Node *parent_{nullptr};
  std::vector<Node *> children_;
};

} // namespace rnwebos::svg
