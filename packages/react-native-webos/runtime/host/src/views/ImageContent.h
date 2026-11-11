#pragma once

#include "ImageFit.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkPaint.h"

#include <optional>

namespace rnwebos {

/// The picture an image view paints: a decoded image fitted in a box, tinted
/// when asked. Everything a frame needs is worked out when the image, the fit
/// or the box changes, so drawing allocates nothing.
class ImageContent final {
 public:
  /// Shows `image`, whose file is `size` pixels: a decode scaled down while
  /// decoding is placed as the file's size asks.
  void setImage(sk_sp<SkImage> image, PixelSize size);
  void clear();
  bool hasImage() const;
  const sk_sp<SkImage> &image() const;
  /// The size of the image's file, or empty without an image.
  PixelSize size() const;

  void setFit(ImageFit fit);
  ImageFit fit() const;
  /// Paints every pixel in `color`, keeping the image's alpha, or stops with no color.
  void setTint(std::optional<SkColor4f> color);
  /// The box the image is fitted in, in the view's pixels, and how many of
  /// them one image pixel covers unscaled.
  void layout(const PixelRect &box, float naturalScale);

  void draw(SkCanvas &canvas, float alpha) const;

 private:
  void place();

  sk_sp<SkImage> image_;
  PixelSize size_;
  ImageFit fit_{ImageFit::Cover};
  PixelRect box_;
  float naturalScale_{1};
  ImagePlacement placement_{};
  SkRect source_{};
  SkPaint paint_;
};

} // namespace rnwebos
