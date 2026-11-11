#include "ImageContent.h"

#include "include/core/SkColorFilter.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkShader.h"

namespace rnwebos {

namespace {

constexpr SkSamplingOptions kSmooth(SkFilterMode::kLinear);

} // namespace

void ImageContent::setImage(sk_sp<SkImage> image, PixelSize size) {
  image_ = std::move(image);
  size_ = size;
  place();
}

void ImageContent::clear() {
  setImage(nullptr, {});
}

bool ImageContent::hasImage() const {
  return image_ != nullptr;
}

const sk_sp<SkImage> &ImageContent::image() const {
  return image_;
}

PixelSize ImageContent::size() const {
  return size_;
}

void ImageContent::setFit(ImageFit fit) {
  if (fit == fit_) return;
  fit_ = fit;
  place();
}

ImageFit ImageContent::fit() const {
  return fit_;
}

void ImageContent::setTint(std::optional<SkColor4f> color) {
  paint_.setColorFilter(color ? SkColorFilters::Blend(*color, nullptr, SkBlendMode::kSrcIn) : nullptr);
}

void ImageContent::layout(const PixelRect &box, float naturalScale) {
  if (box == box_ && naturalScale == naturalScale_) return;
  box_ = box;
  naturalScale_ = naturalScale;
  place();
}

void ImageContent::place() {
  paint_.setShader(nullptr);
  if (!image_) return;
  placement_ = placeImage(fit_, box_, size_, naturalScale_);
  const auto scaleX = static_cast<float>(image_->width()) / static_cast<float>(size_.width);
  const auto scaleY = static_cast<float>(image_->height()) / static_cast<float>(size_.height);
  source_ = SkRect::MakeLTRB(
      placement_.source.left() * scaleX,
      placement_.source.top() * scaleY,
      placement_.source.right() * scaleX,
      placement_.source.bottom() * scaleY);
  if (placement_.tile.isEmpty()) return;
  const auto tile = SkMatrix::ScaleTranslate(
      placement_.tile.width() / static_cast<float>(image_->width()),
      placement_.tile.height() / static_cast<float>(image_->height()),
      placement_.destination.left(),
      placement_.destination.top());
  paint_.setShader(image_->makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat, kSmooth, tile));
}

void ImageContent::draw(SkCanvas &canvas, float alpha) const {
  if (!image_ || placement_.destination.isEmpty()) return;
  SkPaint paint(paint_);
  paint.setAlphaf(alpha);
  if (paint.getShader() != nullptr) {
    canvas.drawRect(placement_.destination, paint);
    return;
  }
  canvas.drawImageRect(image_.get(), source_, placement_.destination, kSmooth, &paint, SkCanvas::kFast_SrcRectConstraint);
}

} // namespace rnwebos
