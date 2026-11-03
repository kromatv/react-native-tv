#include "SvgCanvas.h"

#include <algorithm>
#include <cmath>

namespace rntizen::svg {

namespace {

int toByte(double unit) {
  return static_cast<int>(std::lround(std::clamp(unit, 0.0, 1.0) * 255));
}

struct PremultipliedColor {
  int red{0};
  int green{0};
  int blue{0};
  int alpha{0};
};

PremultipliedColor premultiplied(const std::optional<Rgba> &color) {
  if (!color) return {};
  const auto alpha = std::clamp(color->alpha, 0.0, 1.0);
  return {
      .red = toByte(color->red * alpha),
      .green = toByte(color->green * alpha),
      .blue = toByte(color->blue * alpha),
      .alpha = toByte(alpha),
  };
}

Evas_Vg_Cap capOf(int linecap) {
  switch (linecap) {
    case 1:
      return EVAS_VG_CAP_ROUND;
    case 2:
      return EVAS_VG_CAP_SQUARE;
    default:
      return EVAS_VG_CAP_BUTT;
  }
}

Evas_Vg_Join joinOf(int linejoin) {
  switch (linejoin) {
    case 1:
      return EVAS_VG_JOIN_ROUND;
    case 2:
      return EVAS_VG_JOIN_BEVEL;
    default:
      return EVAS_VG_JOIN_MITER;
  }
}

void appendPath(Evas_Vg_Shape *shape, const Path &path) {
  for (const auto &command : path.commands()) {
    switch (command.kind) {
      case PathCommand::Kind::Move:
        evas_vg_shape_append_move_to(shape, command.to.x, command.to.y);
        break;
      case PathCommand::Kind::Line:
        evas_vg_shape_append_line_to(shape, command.to.x, command.to.y);
        break;
      case PathCommand::Kind::Cubic:
        evas_vg_shape_append_cubic_to(
            shape,
            command.to.x,
            command.to.y,
            command.control1.x,
            command.control1.y,
            command.control2.x,
            command.control2.y);
        break;
      case PathCommand::Kind::Close:
        evas_vg_shape_append_close(shape);
        break;
    }
  }
}

constexpr PremultipliedColor kOpaqueWhite{.red = 255, .green = 255, .blue = 255, .alpha = 255};

void setStops(Evas_Vg_Gradient *gradient, const std::vector<GradientStop> &stops) {
  std::vector<Evas_Vg_Gradient_Stop> evasStops;
  evasStops.reserve(stops.size());
  for (const auto &stop : stops) {
    const auto color = premultiplied(stop.color);
    evasStops.push_back({.offset = stop.offset, .r = color.red, .g = color.green, .b = color.blue, .a = color.alpha});
  }
  evas_vg_gradient_stop_set(gradient, evasStops.data(), static_cast<unsigned int>(evasStops.size()));
}

Evas_Vg_Container *addRoot(Evas_Object *object) {
  auto *root = evas_vg_container_add(object);
  evas_object_vg_root_node_set(object, root);
  return root;
}

} // namespace

SvgCanvas::SvgCanvas(Evas *evas)
    : object_(evas_object_vg_add(evas)),
      root_(addRoot(object_)),
      gradients_(evas_vg_container_add(object_)),
      extent_(evas_vg_shape_add(root_)) {
  evas_object_pass_events_set(object_, EINA_TRUE);
  evas_vg_node_color_set(extent_, 0, 0, 0, 0);
}

SvgCanvas::~SvgCanvas() {
  evas_object_del(object_);
}

Evas_Object *SvgCanvas::object() const {
  return object_;
}

// Evas VG crops its buffer to the root's untransformed path bounds (efl-rendering.md,
// section 8): paths arrive transformed, and `extent_` keeps the buffer view-sized.
void SvgCanvas::paint(const Drawing &drawing, double width, double height) {
  evas_vg_shape_reset(extent_);
  if (width > 0 && height > 0) evas_vg_shape_append_rect(extent_, 0, 0, width, height, 0, 0);
  GradientCursor cursor;
  for (std::size_t index = 0; index < drawing.size(); ++index) paintShape(shapeAt(index), drawing[index], cursor);
  for (std::size_t index = drawing.size(); index < shapes_.size(); ++index) {
    evas_vg_shape_reset(shapes_[index]);
    evas_vg_node_visible_set(shapes_[index], EINA_FALSE);
  }
}

void SvgCanvas::paintShape(Evas_Vg_Shape *shape, const DrawnShape &drawn, GradientCursor &cursor) {
  evas_vg_shape_reset(shape);
  appendPath(shape, drawn.path);
  auto *fill = gradientFor(drawn.fillGradient, cursor);
  evas_vg_shape_fill_set(shape, fill);
  const auto fillColor = fill != nullptr ? kOpaqueWhite : premultiplied(drawn.fill);
  evas_vg_node_color_set(shape, fillColor.red, fillColor.green, fillColor.blue, fillColor.alpha);
  auto *strokeFill = gradientFor(drawn.strokeGradient, cursor);
  evas_vg_shape_stroke_fill_set(shape, strokeFill);
  const auto stroke = strokeFill != nullptr ? kOpaqueWhite : premultiplied(drawn.stroke);
  evas_vg_shape_stroke_color_set(shape, stroke.red, stroke.green, stroke.blue, stroke.alpha);
  const bool stroked = drawn.stroke || drawn.strokeGradient;
  evas_vg_shape_stroke_width_set(shape, stroked ? drawn.strokeWidth : 0);
  evas_vg_shape_stroke_cap_set(shape, capOf(drawn.strokeLinecap));
  evas_vg_shape_stroke_join_set(shape, joinOf(drawn.strokeLinejoin));
  evas_vg_node_visible_set(shape, EINA_TRUE);
}

Evas_Vg_Gradient *SvgCanvas::gradientFor(const std::optional<GradientPaint> &paint, GradientCursor &cursor) {
  if (!paint) return nullptr;
  if (paint->radial) {
    while (radials_.size() <= cursor.radial) radials_.push_back(evas_vg_gradient_radial_add(gradients_));
    auto *gradient = radials_[cursor.radial++];
    evas_vg_gradient_radial_center_set(gradient, paint->center.x, paint->center.y);
    evas_vg_gradient_radial_focal_set(gradient, paint->focal.x, paint->focal.y);
    evas_vg_gradient_radial_radius_set(gradient, paint->radius);
    setStops(gradient, paint->stops);
    return gradient;
  }
  while (linears_.size() <= cursor.linear) linears_.push_back(evas_vg_gradient_linear_add(gradients_));
  auto *gradient = linears_[cursor.linear++];
  evas_vg_gradient_linear_start_set(gradient, paint->start.x, paint->start.y);
  evas_vg_gradient_linear_end_set(gradient, paint->end.x, paint->end.y);
  setStops(gradient, paint->stops);
  return gradient;
}

Evas_Vg_Shape *SvgCanvas::shapeAt(std::size_t index) {
  while (shapes_.size() <= index) shapes_.push_back(evas_vg_shape_add(root_));
  return shapes_[index];
}

} // namespace rntizen::svg
