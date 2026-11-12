#pragma once

#include "PixelGeometry.h"

#include <Evas.h>

namespace rntizen {

/// What is drawn behind a box, blurred: an Evas snapshot image under a `blur`
/// filter. Evas keeps the blurred picture until something under the snapshot
/// changes, so a still backdrop costs nothing per frame; a changing one is
/// captured and blurred again in each frame it changes (efl-rendering.md,
/// "Backdrop blur"). Main thread only.
class BackdropBlur final {
 public:
  explicit BackdropBlur(Evas *evas);
  ~BackdropBlur();

  BackdropBlur(const BackdropBlur &) = delete;
  BackdropBlur &operator=(const BackdropBlur &) = delete;

  /// The snapshot to stack under the box's content and clip to the box, or
  /// null when this Evas has no filters.
  Evas_Object *object() const;
  /// The blur's reach in pixels; 0 hides the snapshot.
  void setRadius(int pixels);
  /// Covers `box` and the radius around it, so the blur meets the box's edges
  /// with the pixels beyond them rather than fading out.
  void layout(const PixelRect &box);

 private:
  void applyVisibility();

  Evas_Object *const snapshot_;
  PixelRect box_;
  int radius_{0};
};

} // namespace rntizen
