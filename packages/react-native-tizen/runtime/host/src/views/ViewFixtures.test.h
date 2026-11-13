#pragma once

#include "ComponentView.h"

#include <doctest/doctest.h>
#include <react/renderer/components/view/ViewProps.h>

#include <Ecore_Evas.h>

#include <cstdint>
#include <cstdlib>
#include <memory>

namespace rntizen::testing {

inline constexpr std::uint32_t kBlack = 0xff000000;
inline constexpr std::uint32_t kRed = 0xffff0000;

/// Layout for a view at (`x`, `y`) of its parent, `width` x `height` points,
/// one pixel a point.
inline facebook::react::LayoutMetrics frameOf(
    facebook::react::Float x,
    facebook::react::Float y,
    facebook::react::Float width,
    facebook::react::Float height) {
  facebook::react::LayoutMetrics metrics;
  metrics.frame = {.origin = {.x = x, .y = y}, .size = {.width = width, .height = height}};
  return metrics;
}

/// View props, default but for what `change` sets.
template <typename Change>
facebook::react::Props::Shared propsWith(Change change) {
  auto props = std::make_shared<facebook::react::ViewProps>();
  change(*props);
  return props;
}

/// View props with nothing but an opaque background color.
inline facebook::react::Props::Shared filled(int red, int green, int blue) {
  return propsWith([&](facebook::react::ViewProps &props) {
    props.backgroundColor = facebook::react::colorFromRGBA(red, green, blue, 255);
  });
}

/// Gives `view` its props and its layout, as mounting does.
inline void place(ComponentView &view, const facebook::react::Props::Shared &props, const facebook::react::LayoutMetrics &layout) {
  view.updateProps(props);
  view.updateLayout(layout);
}

/// Whether two ARGB pixels are the same color, give or take what smooth
/// scaling rounds away.
inline bool isNear(std::uint32_t pixel, std::uint32_t color) {
  for (int shift = 0; shift < 32; shift += 8) {
    if (std::abs(static_cast<int>(pixel >> shift & 0xff) - static_cast<int>(color >> shift & 0xff)) > 3) return false;
  }
  return true;
}

/// A square buffer canvas, drawn when a pixel is read.
class TestCanvas final {
 public:
  explicit TestCanvas(int size) : size_(size), canvas_(newCanvas(size), ecore_evas_free) {}

  Evas *evas() const {
    return ecore_evas_get(canvas_.get());
  }

  /// The ARGB pixel at (`x`, `y`), once the canvas drew what changed.
  std::uint32_t pixel(int x, int y) const {
    ecore_evas_manual_render(canvas_.get());
    return static_cast<const std::uint32_t *>(ecore_evas_buffer_pixels_get(canvas_.get()))[y * size_ + x];
  }

 private:
  static Ecore_Evas *newCanvas(int size) {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    return ecore_evas_buffer_new(size, size);
  }

  const int size_;
  const std::unique_ptr<Ecore_Evas, decltype(&ecore_evas_free)> canvas_;
};

} // namespace rntizen::testing
