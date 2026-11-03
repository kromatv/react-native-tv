#pragma once

#include <optional>
#include <vector>

namespace rntizen::svg {

struct Point {
  double x{0};
  double y{0};
};

/// An axis-aligned rectangle in user units.
struct Bounds {
  double x{0};
  double y{0};
  double width{0};
  double height{0};
};

/// An affine transform with SVG's `matrix(a, b, c, d, e, f)` layout.
struct Affine {
  double a{1};
  double b{0};
  double c{0};
  double d{1};
  double e{0};
  double f{0};

  static Affine translation(double x, double y);
  static Affine scaling(double x, double y);

  Point apply(Point point) const;
  /// `other` applied first, then this.
  Affine operator*(const Affine &other) const;
  /// How much the transform scales lengths on average, for stroke widths.
  double lengthScale() const;
};

struct PathCommand {
  enum class Kind { Move, Line, Cubic, Close };

  Kind kind{Kind::Move};
  Point to;
  Point control1;
  Point control2;
};

/// A path in the four commands Evas VG draws through its public API: arcs and
/// quadratic curves become cubic Béziers as they are appended.
class Path {
 public:
  void moveTo(Point to);
  void lineTo(Point to);
  void cubicTo(Point control1, Point control2, Point to);
  void quadTo(Point control, Point to);
  /// SVG's elliptical arc, from the current point (SVG 1.1 appendix F.6).
  void arcTo(double rx, double ry, double xAxisRotation, bool largeArc, bool sweep, Point to);
  void close();

  Point current() const;
  bool empty() const;
  const std::vector<PathCommand> &commands() const;
  Path transformed(const Affine &transform) const;
  /// The box around every point the path passes through or bends towards.
  Bounds bounds() const;

 private:
  std::vector<PathCommand> commands_;
  Point current_;
  Point subpathStart_;
};

Path ellipsePath(Point center, double rx, double ry);
/// A rectangle with SVG's corner rules: a missing radius takes the other's
/// value, and each is clamped to half the side it rounds.
Path rectPath(Point origin, double width, double height, std::optional<double> rx, std::optional<double> ry);
Path linePath(Point from, Point to);

} // namespace rntizen::svg
