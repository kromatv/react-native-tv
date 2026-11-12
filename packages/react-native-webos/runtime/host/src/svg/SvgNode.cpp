#include "SvgNode.h"

#include "SvgClipPathNode.h"
#include "SvgMaskNode.h"

#include <algorithm>
#include <cmath>

namespace rnwebos::svg {

namespace {

constexpr int kDeepestReference = 16;

} // namespace

Definitions::Definitions(const Node &root) {
  collect(root);
}

void Definitions::collect(const Node &node) {
  if (!node.name.empty()) nodes_.try_emplace(node.name, &node);
  for (const auto *child : node.children()) collect(*child);
}

const Node *Definitions::find(std::string_view name) const {
  if (name.empty()) return nullptr;
  const auto found = nodes_.find(name);
  return found == nodes_.end() ? nullptr : found->second;
}

float diagonalOf(SkSize viewport) {
  return std::sqrt((viewport.width() * viewport.width() + viewport.height() * viewport.height()) / 2);
}

float RenderContext::viewportDiagonal() const {
  return diagonalOf(viewport);
}

bool RenderContext::canFollowReference() const {
  return definitions != nullptr && depth < kDeepestReference;
}

Node::~Node() {
  for (auto *child : children_) child->parent_ = nullptr;
  if (parent_ == nullptr) return;
  std::erase(parent_->children_, this);
  parent_->invalidate();
}

Node *Node::parent() const {
  return parent_;
}

std::span<Node *const> Node::children() const {
  return children_;
}

void Node::setChildren(std::vector<Node *> children) {
  for (auto *child : children_) child->parent_ = nullptr;
  for (auto *child : children) {
    if (child->parent_ != nullptr && child->parent_ != this) {
      std::erase(child->parent_->children_, child);
      child->parent_->invalidate();
    }
    child->parent_ = this;
  }
  children_ = std::move(children);
  invalidate();
}

void Node::invalidate() {
  auto *top = this;
  while (top->parent_ != nullptr) top = top->parent_;
  top->changed();
}

RenderContext Node::inherited(const RenderContext &parent) const {
  RenderContext context{
      .paint = inheritPaint(paint, parent.paint),
      .currentColor = color.value_or(parent.currentColor),
      .font = parent.font,
      .text = parent.text,
      .viewport = parent.viewport,
      .alpha = parent.alpha,
      .definitions = parent.definitions,
      .depth = parent.depth,
  };
  inherit(context);
  return context;
}

bool Node::skipped() const {
  return hidden || opacity <= 0;
}

void Node::render(SkCanvas &canvas, const RenderContext &parent) const {
  if (skipped()) return;
  auto context = inherited(parent);
  const auto *clip = context.definitions != nullptr ? context.definitions->findAs<ClipPathNode>(clipPath) : nullptr;
  const SkAutoCanvasRestore restore(&canvas, clip != nullptr || !matrix.isIdentity());
  canvas.concat(matrix);
  if (clip != nullptr) canvas.clipPath(clip->outline(clipRule, context), true);
  const auto *masking = context.canFollowReference() ? context.definitions->findAs<MaskNode>(mask) : nullptr;
  const float layerAlpha = std::min(opacity, 1.0F) * context.alpha;
  if (masking != nullptr) {
    context.alpha = 1;
    canvas.saveLayerAlphaf(nullptr, layerAlpha);
    draw(canvas, context);
    masking->cut(canvas, contentBounds(context), context);
    canvas.restore();
    return;
  }
  const bool layered = opacity < 1 && contentPaintCount(context) > 1;
  if (layered) canvas.saveLayerAlphaf(nullptr, layerAlpha);
  context.alpha = layered ? 1 : layerAlpha;
  draw(canvas, context);
  if (layered) canvas.restore();
}

int Node::paintCount(const RenderContext &parent) const {
  if (skipped()) return 0;
  if (opacity < 1 || !mask.empty()) return 1;
  return contentPaintCount(inherited(parent));
}

SkRect Node::bounds(const RenderContext &parent) const {
  if (hidden) return SkRect::MakeEmpty();
  return contentBounds(inherited(parent));
}

void Node::appendOutlines(std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &parent) const {
  if (hidden) return;
  appendContentOutlines(outlines, SkMatrix::Concat(transform, matrix), inherited(parent));
}

bool Node::definesOnly() const {
  return false;
}

void Node::draw(SkCanvas &, const RenderContext &) const {
  // A node that only defines something draws nothing itself.
}

int Node::contentPaintCount(const RenderContext &) const {
  return 0;
}

SkRect Node::contentBounds(const RenderContext &) const {
  return SkRect::MakeEmpty();
}

void Node::appendContentOutlines(std::vector<SkPath> &, const SkMatrix &, const RenderContext &) const {
  // A node without geometry adds nothing to a clip.
}

void Node::inherit(RenderContext &) const {
  // Most nodes hand down nothing but paint and color.
}

void Node::changed() {
  // Only the root of a drawn tree has someone to tell.
}

} // namespace rnwebos::svg
