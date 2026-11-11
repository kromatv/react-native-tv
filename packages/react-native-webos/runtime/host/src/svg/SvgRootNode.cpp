#include "SvgRootNode.h"

#include "include/core/SkColorSpace.h"
#include "include/core/SkPictureRecorder.h"

#include <cmath>

namespace rnwebos::svg {

namespace {

constexpr int kPlaysBeforeRaster = 3;
constexpr int kLargestRasterArea = 512 * 512;

bool mapsPixelsOntoPixels(const SkMatrix &matrix) {
  if (!matrix.isTranslate()) return false;
  return matrix.getTranslateX() == std::round(matrix.getTranslateX()) &&
      matrix.getTranslateY() == std::round(matrix.getTranslateY());
}

} // namespace

sk_sp<SkPicture> RootNode::record(const Box &box) const {
  const Definitions definitions(*this);
  RenderContext context{
      .paint = paint,
      .currentColor = color.value_or(SkColor4f{0, 0, 0, 1}),
      .viewport = viewBox.present() ? SkSize::Make(viewBox.width, viewBox.height) : box.size,
      .definitions = &definitions,
  };
  inherit(context);
  SkPictureRecorder recorder;
  auto *canvas = recorder.beginRecording(SkRect::MakeWH(box.size.width() * box.scale, box.size.height() * box.scale));
  canvas->scale(box.scale, box.scale);
  canvas->concat(viewBoxTransform(viewBox, box.size.width(), box.size.height()));
  draw(*canvas, context);
  return recorder.finishRecordingAsPicture();
}

sk_sp<SkImage> RootNode::raster(const SkCanvas &canvas) const {
  if (!mapsPixelsOntoPixels(canvas.getTotalMatrix())) return nullptr;
  if (raster_) return raster_;
  if (++plays_ <= kPlaysBeforeRaster || recording_->approximateOpCount() == 0) return nullptr;
  const auto pixels = recording_->cullRect().roundOut().size();
  if (pixels.isEmpty() || pixels.area() > kLargestRasterArea) return nullptr;
  raster_ = SkImages::DeferredFromPicture(
      recording_, pixels, nullptr, nullptr, SkImages::BitDepth::kU8, SkColorSpace::MakeSRGB());
  return raster_;
}

void RootNode::renderInto(SkCanvas &canvas, SkSize size, float scale) const {
  const Box box{.size = size, .scale = scale};
  if (!recording_ || box != recordedFor_) {
    recording_ = record(box);
    recordedFor_ = box;
    raster_.reset();
    plays_ = 0;
  }
  if (const auto image = raster(canvas)) {
    canvas.drawImage(image, 0, 0);
    return;
  }
  canvas.drawPicture(recording_);
}

void RootNode::changed() {
  recording_.reset();
  raster_.reset();
  plays_ = 0;
  if (onChange) onChange();
}

} // namespace rnwebos::svg
