#include "ImageView.h"

#include "../logging/Log.h"
#include "../mounting/ComponentViewRegistry.h"
#include "EvasColor.h"
#include "ImageFit.h"

#include <react/renderer/components/image/ImageProps.h>
#include <react/renderer/components/image/ImageShadowNode.h>

namespace rntizen {

namespace {

std::string filePath(const std::string &uri) {
  return uri.starts_with("file://") ? uri.substr(7) : uri;
}

} // namespace

ImageView::ImageView(Evas *evas, facebook::react::Tag tag) : ComponentView(evas, tag), image_(evas_object_image_add(evas)) {
  evas_object_pass_events_set(image_, EINA_TRUE);
  evas_object_event_callback_add(image_, EVAS_CALLBACK_IMAGE_PRELOADED, onPreloaded, this);
  addMember(image_, true);
  evas_object_hide(image_);
}

ImageView::~ImageView() {
  evas_object_event_callback_del_full(image_, EVAS_CALLBACK_IMAGE_PRELOADED, onPreloaded, this);
  if (decoding_) evas_object_image_preload(image_, EINA_TRUE);
  evas_object_del(image_);
}

void ImageView::updateProps(const facebook::react::Props::Shared &props) {
  const auto &imageProps = static_cast<const facebook::react::ImageProps &>(*props);
  resizeMode_ = imageProps.resizeMode;
  notifiesLoadEvents_ = imageProps.shouldNotifyLoadEvents;
  if (imageProps.tintColor && *imageProps.tintColor) {
    setColor(image_, *imageProps.tintColor);
  } else {
    evas_object_color_set(image_, 255, 255, 255, 255);
  }
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

void ImageView::loadIfNeeded() {
  if (source_.uri == requestedUri_) return;
  if (source_.uri.empty()) {
    requestedUri_.clear();
    fetch_.reset();
    loaded_ = false;
    evas_object_hide(image_);
    return;
  }
  const auto &size = layoutMetrics().frame.size;
  if (size.width <= 0 || size.height <= 0) return;
  startLoad();
}

void ImageView::startLoad() {
  requestedUri_ = source_.uri;
  fetch_.reset();
  loaded_ = false;
  if (decoding_) evas_object_image_preload(image_, EINA_TRUE);
  decoding_ = false;
  evas_object_hide(image_);
  if (const auto *emitter = loadEvents()) emitter->onLoadStart();
  if (isFetchedUri(source_.uri)) {
    fetch_ = fetchImage(source_, [this](FetchedImage fetched) { decode(fetched); });
    return;
  }
  decode({});
}

void ImageView::setSource(const FetchedImage &fetched) {
  if (fetched.bytes) {
    auto &bytes = *fetched.bytes;
    evas_object_image_memfile_set(image_, const_cast<char *>(bytes.data()), static_cast<int>(bytes.size()), nullptr, nullptr);
  } else {
    evas_object_image_file_set(image_, filePath(source_.uri).c_str(), nullptr);
  }
}

void ImageView::decode(const FetchedImage &fetched) {
  if (!fetched.error.empty()) {
    fail(fetched.error, fetched.status);
    return;
  }
  evas_object_image_load_size_set(image_, 0, 0);
  setSource(fetched);
  if (const auto error = evas_object_image_load_error_get(image_); error != EVAS_LOAD_ERROR_NONE) {
    fail(evas_load_error_str(error), fetched.status);
    return;
  }
  evas_object_image_size_get(image_, &imageWidth_, &imageHeight_);
  const auto &box = canvasRect();
  if (box.width > 0 && box.height > 0 && imageWidth_ >= 2 * box.width && imageHeight_ >= 2 * box.height) {
    evas_object_image_load_size_set(image_, box.width, box.height);
    setSource(fetched);
  }
  decoding_ = true;
  evas_object_image_preload(image_, EINA_FALSE);
}

void ImageView::onPreloaded(void *data, Evas *, Evas_Object *, void *) {
  auto &self = *static_cast<ImageView *>(data);
  if (!self.decoding_) return;
  self.decoding_ = false;
  self.finishLoad();
}

void ImageView::finishLoad() {
  if (const auto error = evas_object_image_load_error_get(image_); error != EVAS_LOAD_ERROR_NONE) {
    fail(evas_load_error_str(error), 0);
    return;
  }
  loaded_ = true;
  layoutMembers();
  evas_object_show(image_);
  if (const auto *emitter = loadEvents()) {
    auto loaded = source_;
    const auto scale = loaded.scale > 0 ? loaded.scale : 1;
    loaded.size = {
        .width = static_cast<facebook::react::Float>(imageWidth_) / scale,
        .height = static_cast<facebook::react::Float>(imageHeight_) / scale,
    };
    emitter->onLoad(loaded);
    emitter->onLoadEnd();
  }
}

void ImageView::fail(const std::string &message, int status) {
  log(LogLevel::Warning, "Image " + source_.uri + " failed: " + message);
  loaded_ = false;
  evas_object_hide(image_);
  if (const auto *emitter = loadEvents()) {
    emitter->onError({.error = message, .responseCode = status, .httpResponseHeaders = {}});
    emitter->onLoadEnd();
  }
}

void ImageView::layoutMembers() {
  if (!loaded_) return;
  const auto &box = canvasRect();
  const auto scale = pointScale();
  const auto &insets = layoutMetrics().contentInsets;
  const auto content = inflated(
      box,
      -toPixels(insets.left, scale),
      -toPixels(insets.top, scale),
      -toPixels(insets.right, scale),
      -toPixels(insets.bottom, scale));
  const auto placement = placeImage(resizeMode_, content, imageWidth_, imageHeight_, scale);
  setGeometry(image_, placement.geometry);
  const auto &fill = placement.fill;
  evas_object_image_fill_set(image_, fill.x, fill.y, fill.width, fill.height);
}

RN_TIZEN_COMPONENT_VIEW("Image", ImageView);

} // namespace rntizen
