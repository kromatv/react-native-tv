#pragma once

#include "NinePatch.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>

namespace rntizen {

/// A side of a box.
enum class Side : std::uint8_t { Top, Right, Bottom, Left };

inline constexpr std::array<Side, 4> kSides{Side::Top, Side::Right, Side::Bottom, Side::Left};

/// A set of a box's sides.
class SideSet final {
 public:
  constexpr SideSet() = default;
  constexpr SideSet(std::initializer_list<Side> sides) {
    for (const auto side : sides) bits_ |= bitOf(side);
  }

  static constexpr SideSet all() {
    using enum Side;
    return {Top, Right, Bottom, Left};
  }

  constexpr bool has(Side side) const {
    return (bits_ & bitOf(side)) != std::byte{0};
  }
  constexpr bool isEmpty() const {
    return bits_ == std::byte{0};
  }
  /// The sides of this set and those of `other`.
  constexpr SideSet with(SideSet other) const {
    SideSet both;
    both.bits_ = bits_ | other.bits_;
    return both;
  }
  /// One bit per side, as a patch key stores the set.
  constexpr std::uint8_t bits() const {
    return std::to_integer<std::uint8_t>(bits_);
  }

  constexpr bool operator==(const SideSet &) const = default;

 private:
  static constexpr std::byte bitOf(Side side) {
    return std::byte{1} << static_cast<int>(side);
  }

  std::byte bits_{0};
};

/// What one image draws of a border whose sides differ in color: its `sides`,
/// cut where each joins a neighbour. Where that neighbour is in `under`, an
/// opaque image above draws it, so this one fills the pixels the join crosses
/// and nothing shows through the seam.
struct RingPart {
  SideSet sides{SideSet::all()};
  SideSet under;

  bool operator==(const RingPart &) const = default;
  /// Whether the part is the ring itself, with no join to cut.
  bool isWhole() const;
};

/// Where the sides of a border of `widths` join in a `width` x `height` image
/// of it whose corner pieces reach `reach` pixels in from each side. Two sides
/// join on the line from the border box's corner through the padding box's
/// corner (efl-rendering.md, section 4).
struct SideJoins {
  Edges widths;
  Edges reach;
  int width{0};
  int height{0};

  /// The side the point (`x`, `y`) belongs to; none between the corner pieces
  /// of both axes, where no border reaches.
  std::optional<Side> sideAt(float x, float y) const;
};

/// How much of the pixel at (`pixelX`, `pixelY`) `part` draws, from 0 to 1,
/// sampled 4 x 4 as `RoundedRect::coverage` does. `inRing(x, y)` says whether
/// a point lies in the border.
template <typename InRing>
float partCoverage(const SideJoins &joins, const RingPart &part, int pixelX, int pixelY, InRing inRing) {
  constexpr int kSamples = 4;
  int own = 0;
  int under = 0;
  for (int i = 0; i < kSamples; ++i) {
    for (int j = 0; j < kSamples; ++j) {
      const auto x = static_cast<float>(pixelX) + (static_cast<float>(i) + 0.5f) / kSamples;
      const auto y = static_cast<float>(pixelY) + (static_cast<float>(j) + 0.5f) / kSamples;
      const auto side = inRing(x, y) ? joins.sideAt(x, y) : std::nullopt;
      if (!side.has_value()) continue;
      own += part.sides.has(*side) ? 1 : 0;
      under += part.under.has(*side) ? 1 : 0;
    }
  }
  return own == 0 ? 0 : static_cast<float>(own + under) / (kSamples * kSamples);
}

} // namespace rntizen
