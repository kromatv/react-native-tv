#include "PathData.h"

#include "include/core/SkPathBuilder.h"
#include "include/core/SkPathTypes.h"

#include <cctype>
#include <charconv>
#include <cmath>
#include <optional>

namespace rnwebos::svg {

namespace {

class Scanner {
 public:
  explicit Scanner(std::string_view text) : text_(text) {}

  void skipSeparators() {
    while (position_ < text_.size() && (isSpace(text_[position_]) || text_[position_] == ',')) ++position_;
  }

  bool atEnd() {
    skipSeparators();
    return position_ >= text_.size();
  }

  std::optional<char> command() {
    skipSeparators();
    if (position_ >= text_.size() || !std::isalpha(static_cast<unsigned char>(text_[position_]))) return std::nullopt;
    return text_[position_++];
  }

  bool startsNumber() {
    skipSeparators();
    if (position_ >= text_.size()) return false;
    const char next = text_[position_];
    return std::isdigit(static_cast<unsigned char>(next)) || next == '-' || next == '+' || next == '.';
  }

  std::optional<double> number() {
    skipSeparators();
    const auto start = position_;
    if (position_ < text_.size() && text_[position_] == '+') ++position_;
    const auto unsignedStart = position_;
    if (position_ < text_.size() && text_[position_] == '-') ++position_;
    const auto integerDigits = digits();
    std::size_t fractionDigits = 0;
    if (position_ < text_.size() && text_[position_] == '.') {
      ++position_;
      fractionDigits = digits();
    }
    if (integerDigits + fractionDigits == 0) {
      position_ = start;
      return std::nullopt;
    }
    exponent();
    double value = 0;
    const auto *first = text_.data() + unsignedStart;
    std::from_chars(first, text_.data() + position_, value);
    return value;
  }

  std::optional<bool> flag() {
    skipSeparators();
    if (position_ >= text_.size() || (text_[position_] != '0' && text_[position_] != '1')) return std::nullopt;
    return text_[position_++] == '1';
  }

 private:
  static bool isSpace(char character) {
    return character == ' ' || character == '\t' || character == '\n' || character == '\r' || character == '\f';
  }

  std::size_t digits() {
    const auto start = position_;
    while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
    return position_ - start;
  }

  void exponent() {
    if (position_ >= text_.size() || (text_[position_] != 'e' && text_[position_] != 'E')) return;
    auto lookahead = position_ + 1;
    if (lookahead < text_.size() && (text_[lookahead] == '-' || text_[lookahead] == '+')) ++lookahead;
    if (lookahead >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[lookahead]))) return;
    position_ = lookahead;
    digits();
  }

  std::string_view text_;
  std::size_t position_{0};
};

struct Position {
  double x{0};
  double y{0};

  bool operator==(const Position &) const = default;
};

SkPoint toPoint(Position position) {
  return {static_cast<float>(position.x), static_cast<float>(position.y)};
}

class PathReader {
 public:
  explicit PathReader(std::string_view data) : scanner_(data) {}

  SkPath read() {
    while (!scanner_.atEnd()) {
      const auto command = scanner_.command();
      if (!command.has_value() || !apply(*command)) break;
    }
    return path_.detach();
  }

 private:
  bool apply(char command) {
    const bool relative = std::islower(static_cast<unsigned char>(command)) != 0;
    const auto upper = static_cast<char>(std::toupper(static_cast<unsigned char>(command)));
    if (!started_ && upper != 'M') return false;
    if (upper == 'Z') {
      path_.close();
      current_ = start_;
      lastControl_.reset();
      return true;
    }
    bool first = true;
    do {
      if (!segment(upper == 'M' && !first ? 'L' : upper, relative)) return false;
      first = false;
    } while (scanner_.startsNumber());
    return true;
  }

  std::optional<Position> point(bool relative) {
    const auto x = scanner_.number();
    const auto y = x.has_value() ? scanner_.number() : std::nullopt;
    if (!y.has_value()) return std::nullopt;
    const auto origin = relative ? current_ : Position{};
    return Position{.x = origin.x + *x, .y = origin.y + *y};
  }

  Position reflectedControl(char expected) const {
    if (!lastControl_ || lastKind_ != expected) return current_;
    return {.x = 2 * current_.x - lastControl_->x, .y = 2 * current_.y - lastControl_->y};
  }

  bool segment(char kind, bool relative) {
    switch (kind) {
      case 'M':
        return moveSegment(relative);
      case 'L':
        return lineSegment(relative);
      case 'H':
      case 'V':
        return axisSegment(kind == 'H', relative);
      case 'C':
      case 'S':
        return cubicSegment(kind == 'S', relative);
      case 'Q':
      case 'T':
        return quadSegment(kind == 'T', relative);
      case 'A':
        return arcSegment(relative);
      default:
        return false;
    }
  }

  bool moveSegment(bool relative) {
    const auto to = point(relative);
    if (!to.has_value()) return false;
    path_.moveTo(toPoint(*to));
    current_ = *to;
    start_ = *to;
    started_ = true;
    lastControl_.reset();
    return true;
  }

  void lineTo(Position to) {
    path_.lineTo(toPoint(to));
    current_ = to;
    lastControl_.reset();
  }

  bool lineSegment(bool relative) {
    const auto to = point(relative);
    if (!to.has_value()) return false;
    lineTo(*to);
    return true;
  }

  bool axisSegment(bool horizontal, bool relative) {
    const auto value = scanner_.number();
    if (!value.has_value()) return false;
    auto to = current_;
    auto &coordinate = horizontal ? to.x : to.y;
    coordinate = relative ? coordinate + *value : *value;
    lineTo(to);
    return true;
  }

  bool cubicSegment(bool smooth, bool relative) {
    const auto control1 = smooth ? std::optional(reflectedControl('C')) : point(relative);
    const auto control2 = control1.has_value() ? point(relative) : std::nullopt;
    const auto to = control2.has_value() ? point(relative) : std::nullopt;
    if (!to.has_value()) return false;
    path_.cubicTo(toPoint(*control1), toPoint(*control2), toPoint(*to));
    current_ = *to;
    lastControl_ = control2;
    lastKind_ = 'C';
    return true;
  }

  bool quadSegment(bool smooth, bool relative) {
    const auto control = smooth ? std::optional(reflectedControl('Q')) : point(relative);
    const auto to = control.has_value() ? point(relative) : std::nullopt;
    if (!to.has_value()) return false;
    path_.quadTo(toPoint(*control), toPoint(*to));
    current_ = *to;
    lastControl_ = control;
    lastKind_ = 'Q';
    return true;
  }

  bool arcSegment(bool relative) {
    const auto rx = scanner_.number();
    const auto ry = rx.has_value() ? scanner_.number() : std::nullopt;
    const auto rotation = ry.has_value() ? scanner_.number() : std::nullopt;
    const auto largeArc = rotation.has_value() ? scanner_.flag() : std::nullopt;
    const auto sweep = largeArc.has_value() ? scanner_.flag() : std::nullopt;
    const auto to = sweep.has_value() ? point(relative) : std::nullopt;
    if (!to.has_value()) return false;
    // SVG leaves out an arc that ends where it starts; Skia's arcTo would add a line there.
    if (*to == current_) return true;
    path_.arcTo(
        {static_cast<float>(std::abs(*rx)), static_cast<float>(std::abs(*ry))},
        static_cast<float>(*rotation),
        *largeArc ? SkPathBuilder::kLarge_ArcSize : SkPathBuilder::kSmall_ArcSize,
        *sweep ? SkPathDirection::kCW : SkPathDirection::kCCW,
        toPoint(*to));
    current_ = *to;
    lastControl_.reset();
    return true;
  }

  Scanner scanner_;
  SkPathBuilder path_;
  Position current_;
  Position start_;
  bool started_{false};
  std::optional<Position> lastControl_;
  char lastKind_{0};
};

} // namespace

SkPath parsePathData(std::string_view data) {
  return PathReader(data).read();
}

} // namespace rnwebos::svg
