#pragma once

#include "SvgGroupNode.h"
#include "SvgViewBox.h"

#include "include/core/SkImage.h"
#include "include/core/SkPicture.h"

#include <functional>

namespace rnwebos::svg {

/// The root of the nodes one `<Svg>` draws: its viewBox, the color
/// `currentColor` stands for (`color`), and who to tell when the tree changes.
///
/// The tree is walked once into a picture, which every frame plays until a
/// node reports a change or the box it is drawn into becomes another. A
/// picture played a few frames in a row where its pixels fall on the canvas's
/// own is then kept as an image, so a still icon costs one textured
/// rectangle a frame whatever its paths; under a scale, a rotation or a
/// fractional offset the picture is played, which stays sharp.
class RootNode final : public GroupNode {
 public:
  SvgViewBox viewBox;
  std::function<void()> onChange;

  /// Draws the tree at the canvas's origin into a box of `size` layout
  /// points, `scale` pixels to the point. User units are points; a viewBox
  /// maps its own units into the box.
  void renderInto(SkCanvas &canvas, SkSize size, float scale) const;

 protected:
  void changed() override;

 private:
  struct Box {
    SkSize size;
    float scale{1};

    bool operator==(const Box &) const = default;
  };

  sk_sp<SkPicture> record(const Box &box) const;
  /// The picture as an image, once it is worth one and `canvas` shows it
  /// pixel for pixel; null to play the picture.
  sk_sp<SkImage> raster(const SkCanvas &canvas) const;

  mutable sk_sp<SkPicture> recording_;
  mutable Box recordedFor_;
  mutable sk_sp<SkImage> raster_;
  mutable int plays_{0};
};

} // namespace rnwebos::svg
