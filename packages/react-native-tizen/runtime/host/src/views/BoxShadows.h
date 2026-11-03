#pragma once

#include "BoxStyle.h"
#include "PixelGeometry.h"

#include <Evas.h>

#include <memory>
#include <vector>

namespace rntizen {

/// A view's drop shadows: one blurred nine-patch per shadow, under its
/// background. A patch not made yet is made off the main loop; until it
/// arrives the shadow keeps the patch it showed, or shows nothing.
class BoxShadows final {
 public:
  BoxShadows(Evas *evas, Evas_Object *smart);
  ~BoxShadows();

  BoxShadows(const BoxShadows &) = delete;
  BoxShadows &operator=(const BoxShadows &) = delete;

  /// Returns true when shadows were added or removed.
  bool update(const std::vector<ShadowStyle> &shadows, const Corners &corners);
  void layout(const PixelRect &box);
  const std::vector<Evas_Object *> &objects() const;
  /// How far the shadows reach past the box on each side.
  Edges overflow() const;

 private:
  void showPatch(std::size_t index, const Corners &corners, int blur);

  Evas *const evas_;
  Evas_Object *const smart_;
  std::vector<Evas_Object *> images_;
  std::vector<ShadowStyle> shadows_;
  Corners corners_;
  std::shared_ptr<BoxShadows *> alive_;
};

} // namespace rntizen
