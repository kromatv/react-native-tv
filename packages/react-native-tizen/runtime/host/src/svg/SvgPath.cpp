#include "SvgPath.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace rntizen::svg {

namespace {

constexpr double kQuarterTurn = std::numbers::pi / 2;
constexpr double kCircleKappa = 0.5522847498307936;

double vectorAngle(Point from, Point to) {
  return std::atan2(from.x * to.y - from.y * to.x, from.x * to.x + from.y * to.y);
}

bool samePoint(Point left, Point right) {
  return left.x == right.x && left.y == right.y;
}

} // namespace

Affine Affine::translation(double x, double y) {
  return {.e = x, .f = y};
}

Affine Affine::scaling(double x, double y) {
  return {.a = x, .d = y};
}

Point Affine::apply(Point point) const {
  return {.x = a * point.x + c * point.y + e, .y = b * point.x + d * point.y + f};
}

Affine Affine::operator*(const Affine &other) const {
  return {
      .a = a * other.a + c * other.b,
      .b = b * other.a + d * other.b,
      .c = a * other.c + c * other.d,
      .d = b * other.c + d * other.d,
      .e = a * other.e + c * other.f + e,
      .f = b * other.e + d * other.f + f,
  };
}

double Affine::lengthScale() const {
  return std::sqrt(std::abs(a * d - b * c));
}

void Path::moveTo(Point to) {
  commands_.push_back({.kind = PathCommand::Kind::Move, .to = to});
  current_ = to;
  subpathStart_ = to;
}

void Path::lineTo(Point to) {
  commands_.push_back({.kind = PathCommand::Kind::Line, .to = to});
  current_ = to;
}

void Path::cubicTo(Point control1, Point control2, Point to) {
  commands_.push_back({.kind = PathCommand::Kind::Cubic, .to = to, .control1 = control1, .control2 = control2});
  current_ = to;
}

void Path::quadTo(Point control, Point to) {
  const auto from = current_;
  cubicTo(
      {.x = from.x + 2.0 / 3.0 * (control.x - from.x), .y = from.y + 2.0 / 3.0 * (control.y - from.y)},
      {.x = to.x + 2.0 / 3.0 * (control.x - to.x), .y = to.y + 2.0 / 3.0 * (control.y - to.y)},
      to);
}

void Path::arcTo(double rx, double ry, double xAxisRotation, bool largeArc, bool sweep, Point to) {
  const auto from = current_;
  if (samePoint(from, to)) return;
  rx = std::abs(rx);
  ry = std::abs(ry);
  if (rx == 0 || ry == 0) {
    lineTo(to);
    return;
  }
  const double phi = xAxisRotation * std::numbers::pi / 180;
  const double cosPhi = std::cos(phi);
  const double sinPhi = std::sin(phi);
  const double halfDx = (from.x - to.x) / 2;
  const double halfDy = (from.y - to.y) / 2;
  const double x1 = cosPhi * halfDx + sinPhi * halfDy;
  const double y1 = -sinPhi * halfDx + cosPhi * halfDy;

  const double radiiScale = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry);
  if (radiiScale > 1) {
    rx *= std::sqrt(radiiScale);
    ry *= std::sqrt(radiiScale);
  }
  const double numerator = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1;
  const double denominator = rx * rx * y1 * y1 + ry * ry * x1 * x1;
  const double root = std::sqrt(std::max(0.0, numerator / denominator)) * (largeArc == sweep ? -1 : 1);
  const double centerX1 = root * rx * y1 / ry;
  const double centerY1 = -root * ry * x1 / rx;
  const double centerX = cosPhi * centerX1 - sinPhi * centerY1 + (from.x + to.x) / 2;
  const double centerY = sinPhi * centerX1 + cosPhi * centerY1 + (from.y + to.y) / 2;

  const Point startVector{.x = (x1 - centerX1) / rx, .y = (y1 - centerY1) / ry};
  const Point endVector{.x = (-x1 - centerX1) / rx, .y = (-y1 - centerY1) / ry};
  const double startAngle = vectorAngle({.x = 1, .y = 0}, startVector);
  double sweepAngle = vectorAngle(startVector, endVector);
  if (!sweep && sweepAngle > 0) sweepAngle -= 2 * std::numbers::pi;
  if (sweep && sweepAngle < 0) sweepAngle += 2 * std::numbers::pi;

  const auto segments = std::max(1, static_cast<int>(std::ceil(std::abs(sweepAngle) / kQuarterTurn - 1e-9)));
  const double step = sweepAngle / segments;
  const double handle = 4.0 / 3.0 * std::tan(step / 4);
  const auto onEllipse = [&](double unitX, double unitY) {
    return Point{
        .x = centerX + rx * cosPhi * unitX - ry * sinPhi * unitY,
        .y = centerY + rx * sinPhi * unitX + ry * cosPhi * unitY,
    };
  };
  for (int segment = 0; segment < segments; ++segment) {
    const double angle1 = startAngle + segment * step;
    const double angle2 = angle1 + step;
    const double cos1 = std::cos(angle1);
    const double sin1 = std::sin(angle1);
    const double cos2 = std::cos(angle2);
    const double sin2 = std::sin(angle2);
    const auto end = segment == segments - 1 ? to : onEllipse(cos2, sin2);
    cubicTo(onEllipse(cos1 - handle * sin1, sin1 + handle * cos1), onEllipse(cos2 + handle * sin2, sin2 - handle * cos2), end);
  }
}

void Path::close() {
  commands_.push_back({.kind = PathCommand::Kind::Close, .to = subpathStart_});
  current_ = subpathStart_;
}

Point Path::current() const {
  return current_;
}

bool Path::empty() const {
  return commands_.empty();
}

const std::vector<PathCommand> &Path::commands() const {
  return commands_;
}

Path Path::transformed(const Affine &transform) const {
  Path result = *this;
  for (auto &command : result.commands_) {
    command.to = transform.apply(command.to);
    command.control1 = transform.apply(command.control1);
    command.control2 = transform.apply(command.control2);
  }
  result.current_ = transform.apply(current_);
  result.subpathStart_ = transform.apply(subpathStart_);
  return result;
}

Bounds Path::bounds() const {
  constexpr auto kFar = std::numeric_limits<double>::infinity();
  double left = kFar;
  double top = kFar;
  double right = -kFar;
  double bottom = -kFar;
  auto include = [&](Point point) {
    left = std::min(left, point.x);
    top = std::min(top, point.y);
    right = std::max(right, point.x);
    bottom = std::max(bottom, point.y);
  };
  for (const auto &command : commands_) {
    if (command.kind == PathCommand::Kind::Close) continue;
    if (command.kind == PathCommand::Kind::Cubic) {
      include(command.control1);
      include(command.control2);
    }
    include(command.to);
  }
  if (left > right) return {};
  return {.x = left, .y = top, .width = right - left, .height = bottom - top};
}

Path ellipsePath(Point center, double rx, double ry) {
  Path path;
  if (rx <= 0 || ry <= 0) return path;
  const double kx = rx * kCircleKappa;
  const double ky = ry * kCircleKappa;
  const double left = center.x - rx;
  const double right = center.x + rx;
  const double top = center.y - ry;
  const double bottom = center.y + ry;
  path.moveTo({.x = right, .y = center.y});
  path.cubicTo({.x = right, .y = center.y + ky}, {.x = center.x + kx, .y = bottom}, {.x = center.x, .y = bottom});
  path.cubicTo({.x = center.x - kx, .y = bottom}, {.x = left, .y = center.y + ky}, {.x = left, .y = center.y});
  path.cubicTo({.x = left, .y = center.y - ky}, {.x = center.x - kx, .y = top}, {.x = center.x, .y = top});
  path.cubicTo({.x = center.x + kx, .y = top}, {.x = right, .y = center.y - ky}, {.x = right, .y = center.y});
  path.close();
  return path;
}

Path rectPath(Point origin, double width, double height, std::optional<double> rx, std::optional<double> ry) {
  Path path;
  if (width <= 0 || height <= 0) return path;
  const double radiusX = std::clamp(rx.value_or(ry.value_or(0)), 0.0, width / 2);
  const double radiusY = std::clamp(ry.value_or(rx.value_or(0)), 0.0, height / 2);
  const double left = origin.x;
  const double top = origin.y;
  const double right = origin.x + width;
  const double bottom = origin.y + height;
  if (radiusX == 0 || radiusY == 0) {
    path.moveTo({.x = left, .y = top});
    path.lineTo({.x = right, .y = top});
    path.lineTo({.x = right, .y = bottom});
    path.lineTo({.x = left, .y = bottom});
    path.close();
    return path;
  }
  const double kx = radiusX * (1 - kCircleKappa);
  const double ky = radiusY * (1 - kCircleKappa);
  path.moveTo({.x = left + radiusX, .y = top});
  path.lineTo({.x = right - radiusX, .y = top});
  path.cubicTo({.x = right - kx, .y = top}, {.x = right, .y = top + ky}, {.x = right, .y = top + radiusY});
  path.lineTo({.x = right, .y = bottom - radiusY});
  path.cubicTo({.x = right, .y = bottom - ky}, {.x = right - kx, .y = bottom}, {.x = right - radiusX, .y = bottom});
  path.lineTo({.x = left + radiusX, .y = bottom});
  path.cubicTo({.x = left + kx, .y = bottom}, {.x = left, .y = bottom - ky}, {.x = left, .y = bottom - radiusY});
  path.lineTo({.x = left, .y = top + radiusY});
  path.cubicTo({.x = left, .y = top + ky}, {.x = left + kx, .y = top}, {.x = left + radiusX, .y = top});
  path.close();
  return path;
}

Path linePath(Point from, Point to) {
  Path path;
  path.moveTo(from);
  path.lineTo(to);
  return path;
}

} // namespace rntizen::svg
