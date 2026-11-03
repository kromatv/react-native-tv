#include "../mounting/ComponentViewRegistry.h"
#include "PathData.h"
#include "SvgNodeView.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rntizen::svg {

namespace {

Length lengthOr(const folly::dynamic &value, double fallback = 0) {
  return lengthFromProp(value).value_or(Length{.value = fallback});
}

class SvgShapeView : public SvgNodeView {
 public:
  using SvgNodeView::SvgNodeView;

 protected:
  void drawContent(Drawing &drawing, const DrawContext &context) const final {
    const auto path = geometry(context);
    if (!path.empty()) drawing.push_back(drawnShape(path, context));
  }

  virtual Path geometry(const DrawContext &context) const = 0;
};

class SvgPathView final : public SvgShapeView {
 public:
  using SvgShapeView::SvgShapeView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &path = static_cast<const facebook::react::RNSVGPathProps &>(props);
    readRenderable(path);
    if (path.d != data_) {
      data_ = path.d;
      path_ = parsePathData(data_);
    }
  }

  Path geometry(const DrawContext &) const override {
    return path_;
  }

 private:
  std::string data_;
  Path path_;
};

class SvgCircleView final : public SvgShapeView {
 public:
  using SvgShapeView::SvgShapeView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &circle = static_cast<const facebook::react::RNSVGCircleProps &>(props);
    readRenderable(circle);
    cx_ = lengthOr(circle.cx);
    cy_ = lengthOr(circle.cy);
    r_ = lengthOr(circle.r);
  }

  Path geometry(const DrawContext &context) const override {
    const double radius = r_.resolve(context.viewportDiagonal());
    const Point center{.x = cx_.resolve(context.viewportWidth), .y = cy_.resolve(context.viewportHeight)};
    return ellipsePath(center, radius, radius);
  }

 private:
  Length cx_;
  Length cy_;
  Length r_;
};

class SvgEllipseView final : public SvgShapeView {
 public:
  using SvgShapeView::SvgShapeView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &ellipse = static_cast<const facebook::react::RNSVGEllipseProps &>(props);
    readRenderable(ellipse);
    cx_ = lengthOr(ellipse.cx);
    cy_ = lengthOr(ellipse.cy);
    rx_ = lengthOr(ellipse.rx);
    ry_ = lengthOr(ellipse.ry);
  }

  Path geometry(const DrawContext &context) const override {
    const Point center{.x = cx_.resolve(context.viewportWidth), .y = cy_.resolve(context.viewportHeight)};
    return ellipsePath(center, rx_.resolve(context.viewportWidth), ry_.resolve(context.viewportHeight));
  }

 private:
  Length cx_;
  Length cy_;
  Length rx_;
  Length ry_;
};

class SvgRectView final : public SvgShapeView {
 public:
  using SvgShapeView::SvgShapeView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &rect = static_cast<const facebook::react::RNSVGRectProps &>(props);
    readRenderable(rect);
    x_ = lengthOr(rect.x);
    y_ = lengthOr(rect.y);
    width_ = lengthOr(rect.width);
    height_ = lengthOr(rect.height);
    rx_ = lengthFromProp(rect.rx);
    ry_ = lengthFromProp(rect.ry);
  }

  Path geometry(const DrawContext &context) const override {
    const double width = context.viewportWidth;
    const double height = context.viewportHeight;
    return rectPath(
        {.x = x_.resolve(width), .y = y_.resolve(height)},
        width_.resolve(width),
        height_.resolve(height),
        resolved(rx_, width),
        resolved(ry_, height));
  }

 private:
  static std::optional<double> resolved(const std::optional<Length> &length, double reference) {
    if (!length) return std::nullopt;
    return length->resolve(reference);
  }

  Length x_;
  Length y_;
  Length width_;
  Length height_;
  std::optional<Length> rx_;
  std::optional<Length> ry_;
};

class SvgLineView final : public SvgShapeView {
 public:
  using SvgShapeView::SvgShapeView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &line = static_cast<const facebook::react::RNSVGLineProps &>(props);
    readRenderable(line);
    x1_ = lengthOr(line.x1);
    y1_ = lengthOr(line.y1);
    x2_ = lengthOr(line.x2);
    y2_ = lengthOr(line.y2);
  }

  Path geometry(const DrawContext &context) const override {
    const double width = context.viewportWidth;
    const double height = context.viewportHeight;
    return linePath(
        {.x = x1_.resolve(width), .y = y1_.resolve(height)}, {.x = x2_.resolve(width), .y = y2_.resolve(height)});
  }

 private:
  Length x1_;
  Length y1_;
  Length x2_;
  Length y2_;
};

} // namespace

RN_TIZEN_COMPONENT_VIEW("RNSVGPath", SvgPathView);
RN_TIZEN_COMPONENT_VIEW("RNSVGCircle", SvgCircleView);
RN_TIZEN_COMPONENT_VIEW("RNSVGEllipse", SvgEllipseView);
RN_TIZEN_COMPONENT_VIEW("RNSVGRect", SvgRectView);
RN_TIZEN_COMPONENT_VIEW("RNSVGLine", SvgLineView);

} // namespace rntizen::svg
