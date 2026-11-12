#pragma once

#include "SvgNode.h"

#include <optional>
#include <string>
#include <utility>

namespace rnwebos::svg {

/// A node that fills and strokes one path. The path is built from the
/// element's attributes once and kept until they change, so Skia can keep
/// what it derives from it between frames.
class ShapeNode : public Node {
 protected:
  /// The shape in user units, percentages resolved against `viewport`.
  virtual SkPath buildPath(SkSize viewport) const = 0;
  void forgetPath();

  void draw(SkCanvas &canvas, const RenderContext &context) const override;
  int contentPaintCount(const RenderContext &context) const override;
  SkRect contentBounds(const RenderContext &context) const override;
  void appendContentOutlines(
      std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &context) const override;

 private:
  const SkPath &path(const RenderContext &context) const;
  const SkRect &fillBounds(const RenderContext &context) const;

  mutable std::optional<SkPath> path_;
  mutable std::optional<SkRect> bounds_;
  mutable SkSize builtFor_;
};

struct PathGeometry {
  std::string data;

  bool operator==(const PathGeometry &) const = default;
  SkPath path(SkSize viewport) const;
};

struct CircleGeometry {
  Length cx;
  Length cy;
  Length r;

  bool operator==(const CircleGeometry &) const = default;
  SkPath path(SkSize viewport) const;
};

struct EllipseGeometry {
  Length cx;
  Length cy;
  Length rx;
  Length ry;

  bool operator==(const EllipseGeometry &) const = default;
  SkPath path(SkSize viewport) const;
};

struct RectGeometry {
  Length x;
  Length y;
  Length width;
  Length height;
  std::optional<Length> rx;
  std::optional<Length> ry;

  bool operator==(const RectGeometry &) const = default;
  SkPath path(SkSize viewport) const;
};

struct LineGeometry {
  Length x1;
  Length y1;
  Length x2;
  Length y2;

  bool operator==(const LineGeometry &) const = default;
  SkPath path(SkSize viewport) const;
};

/// A shape whose path comes from the attributes in `Geometry`.
template <typename Geometry>
class GeometryNode final : public ShapeNode {
 public:
  /// Takes the element's attributes; the path is built again only when they differ.
  void setGeometry(Geometry geometry) {
    if (geometry == geometry_) return;
    geometry_ = std::move(geometry);
    forgetPath();
  }

 private:
  SkPath buildPath(SkSize viewport) const override {
    return geometry_.path(viewport);
  }

  Geometry geometry_;
};

using PathNode = GeometryNode<PathGeometry>;
using CircleNode = GeometryNode<CircleGeometry>;
using EllipseNode = GeometryNode<EllipseGeometry>;
using RectNode = GeometryNode<RectGeometry>;
using LineNode = GeometryNode<LineGeometry>;

} // namespace rnwebos::svg
