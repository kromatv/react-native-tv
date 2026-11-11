#include "ImageView.h"

#include "../logging/Log.h"
#include "../mounting/ComponentViewRegistry.h"
#include "DecodedImages.h"
#include "SkiaColor.h"

#include <react/renderer/components/image/ImageProps.h>
#include <react/renderer/components/image/ImageShadowNode.h>

#include <optional>
#include <utility>

namespace rnwebos {

namespace {

std::string filePath(const std::string &uri) {
  return uri.starts_with("file://") ? uri.substr(7) : uri;
}

ImageFit fitOf(facebook::react::ImageResizeMode resizeMode) {
  using enum facebook::react::ImageResizeMode;
  switch (resizeMode) {
    case Cover:
      return ImageFit::Cover;
    case Contain:
      return ImageFit::Contain;
    case Stretch:
      return ImageFit::Stretch;
    case Center:
    case None:
      return ImageFit::Center;
    case Repeat:
      return ImageFit::Repeat;
  }
  return ImageFit::Stretch;
}

} // namespace

void ImageView::updateProps(const facebook::react::Props::Shared &props) {
  const auto &imageProps = static_cast<const facebook::react::ImageProps &>(*props);
  content_.setFit(fitOf(imageProps.resizeMode));
  notifiesLoadEvents_ = imageProps.shouldNotifyLoadEvents;
  const bool tinted = imageProps.tintColor && *imageProps.tintColor;
  content_.setTint(tinted ? std::optional{skColor(*imageProps.tintColor)} : std::nullopt);
  if (!hasStateSource_) source_ = imageProps.sources.empty() ? facebook::react::ImageSource{} : imageProps.sources.front();
  ComponentView::updateProps(props);
  updateCorners();
  loadIfNeeded();
}

void ImageView::updateState(const facebook::react::State::Shared &state) {
  if (!state) return;
  const auto &data = std::static_pointer_cast<const facebook::react::ImageShadowNode::ConcreteState>(state)->getData();
  const auto source = data.getImageSource();
  if (source.type == facebook::react::ImageSource::Type::Invalid) return;
  hasStateSource_ = true;
  source_ = source;
  loadIfNeeded();
}

void ImageView::updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) {
  ComponentView::updateLayout(layoutMetrics);
  updateCorners();
  loadIfNeeded();
}

const facebook::react::ImageEventEmitter *ImageView::loadEvents() const {
  if (!notifiesLoadEvents_ || !eventEmitter()) return nullptr;
  return &static_cast<const facebook::react::ImageEventEmitter &>(*eventEmitter());
}

void ImageView::updateCorners() {
  if (!props()) return;
  const auto &radii = static_cast<const facebook::react::ViewProps &>(*props()).resolveBorderMetrics(layoutMetrics()).borderRadii;
  setAlwaysClips(!radii.isUniform() || radii.topLeft.horizontal > 0 || radii.topLeft.vertical > 0);
}

PixelRect ImageView::contentBox() const {
  const auto scale = pointScale();
  const auto size = pixelSize();
  const auto &insets = layoutMetrics().contentInsets;
  return inflated(
      {.x = 0, .y = 0, .width = size.width, .height = size.height},
      -toPixels(insets.left, scale),
      -toPixels(insets.top, scale),
      -toPixels(insets.right, scale),
      -toPixels(insets.bottom, scale));
}

void ImageView::layoutContent() {
  content_.layout(contentBox(), static_cast<float>(pointScale()));
}

int ImageView::contentLayers() const {
  return content_.hasImage() ? 1 : 0;
}

void ImageView::drawContent(SkCanvas &canvas, float alpha) {
  content_.draw(canvas, alpha);
}

void ImageView::loadIfNeeded() {
  if (source_.uri.empty()) {
    if (requestedUri_.empty()) return;
    requestedUri_.clear();
    fetch_.reset();
    decode_.reset();
    content_.clear();
    invalidate();
    return;
  }
  const auto box = contentBox();
  if (box.width <= 0 || box.height <= 0) return;
  if (source_.uri == requestedUri_) {
    sharpenIfNeeded();
  } else {
    startLoad();
  }
}

void ImageView::startLoad() {
  requestedUri_ = source_.uri;
  fetch_.reset();
  decode_.reset();
  sharpening_ = false;
  content_.clear();
  invalidate();
  if (const auto *emitter = loadEvents()) emitter->onLoadStart();
  const auto box = contentBox();
  request({.width = box.width, .height = box.height});
}

// A box that grew past what its image was decoded for gets the image again,
// for twice the box so a box that keeps growing asks only a few times.
void ImageView::sharpenIfNeeded() {
  if (!content_.hasImage() || fetch_ || decode_) return;
  const auto box = contentBox();
  const auto wanted =
      decodedImageSize(content_.fit(), {box.width, box.height}, content_.size(), static_cast<float>(pointScale()));
  const auto &image = content_.image();
  if (image->width() >= wanted.width && image->height() >= wanted.height) return;
  sharpening_ = true;
  request({.width = 2 * box.width, .height = 2 * box.height});
}

void ImageView::request(PixelSize box) {
  ImageDecodeRequest wanted{
      .bytes = nullptr, .path = {}, .fit = content_.fit(), .box = box, .naturalScale = static_cast<float>(pointScale())};
  if (const auto kept = findDecodedImage(requestedUri_, wanted.fit, wanted.box, wanted.naturalScale)) {
    show(*kept);
    return;
  }
  if (!isFetchedUri(source_.uri)) {
    wanted.path = filePath(source_.uri);
    decode(std::move(wanted));
    return;
  }
  fetch_ = fetchImage(source_, [this, wanted](FetchedImage fetched) mutable {
    fetch_.reset();
    if (!fetched.error.empty()) {
      fail(fetched.error, fetched.status);
      return;
    }
    wanted.bytes = std::move(fetched.bytes);
    decode(std::move(wanted));
  });
}

void ImageView::decode(ImageDecodeRequest request) {
  decode_ = decodeImage(std::move(request), [this](DecodedImage decoded) {
    decode_.reset();
    if (!decoded.error.empty()) {
      fail(decoded.error, 0);
      return;
    }
    show(keepDecodedImage(scene(), requestedUri_, std::move(decoded)));
  });
}

void ImageView::show(const DecodedImage &decoded) {
  const bool quiet = std::exchange(sharpening_, false);
  content_.setImage(decoded.image, decoded.size);
  invalidate();
  const auto *emitter = loadEvents();
  if (quiet || emitter == nullptr) return;
  auto loaded = source_;
  const auto scale = loaded.scale > 0 ? loaded.scale : 1;
  loaded.size = {
      .width = static_cast<facebook::react::Float>(decoded.size.width) / scale,
      .height = static_cast<facebook::react::Float>(decoded.size.height) / scale,
  };
  emitter->onLoad(loaded);
  emitter->onLoadEnd();
}

void ImageView::fail(const std::string &message, int status) {
  log(LogLevel::Warning, "Image " + source_.uri + " failed: " + message);
  if (std::exchange(sharpening_, false)) return;
  content_.clear();
  invalidate();
  if (const auto *emitter = loadEvents()) {
    emitter->onError({.error = message, .responseCode = status, .httpResponseHeaders = {}});
    emitter->onLoadEnd();
  }
}

RN_WEBOS_COMPONENT_VIEW("Image", ImageView);

} // namespace rnwebos
