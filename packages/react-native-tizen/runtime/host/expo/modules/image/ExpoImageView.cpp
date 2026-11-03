#include "ExpoImageView.h"

#include "ImageCache.h"

#include "../../WorkQueue.h"
#include "../../views/ViewDefinition.h"

#include "threading/MainThread.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace rntizen::expo {

namespace {

const folly::dynamic *firstSource(const folly::dynamic &value) {
  if (value.isArray() && !value.empty() && value[0].isObject()) return &value[0];
  if (value.isObject()) return &value;
  return nullptr;
}

std::string stringField(const folly::dynamic &object, const char *name) {
  const auto *field = object.get_ptr(name);
  return field != nullptr && field->isString() ? field->getString() : std::string();
}

double fitScale(std::string_view contentFit, double scaleX, double scaleY, double naturalScale) {
  if (contentFit == "cover") return std::max(scaleX, scaleY);
  if (contentFit == "contain") return std::min(scaleX, scaleY);
  if (contentFit == "scale-down") return std::min(naturalScale, std::min(scaleX, scaleY));
  return naturalScale;
}

struct FetchedImage {
  std::string path;
  std::string cacheType;
  std::string error;
};

FetchedImage fetchImage(const std::string &uri, const std::string &cacheKey) {
  try {
    auto file = imageFileFor(uri, cacheKey);
    return {.path = std::move(file.path), .cacheType = std::move(file.cacheType), .error = {}};
  } catch (const std::exception &failure) {
    return {.path = {}, .cacheType = {}, .error = failure.what()};
  }
}

} // namespace

ExpoImageView::ExpoImageView(Evas *evas, facebook::react::Tag tag)
    : ExpoView(evas, tag), image_(evas_object_image_filled_add(evas)), alive_(std::make_shared<ExpoImageView *>(this)) {
  evas_object_pass_events_set(image_, EINA_TRUE);
  addMember(image_, true);
  setAlwaysClips(true);
  evas_object_image_smooth_scale_set(image_, EINA_TRUE);
  evas_object_event_callback_add(image_, EVAS_CALLBACK_IMAGE_PRELOADED, onPreloaded, this);
  evas_object_hide(image_);
}

ExpoImageView::~ExpoImageView() {
  alive_.reset();
  if (fade_ != nullptr) ecore_animator_del(fade_);
  evas_object_event_callback_del_full(image_, EVAS_CALLBACK_IMAGE_PRELOADED, onPreloaded, this);
  cancelDecode();
  evas_object_del(image_);
}

void ExpoImageView::setSource(const folly::dynamic &value) {
  const auto *source = firstSource(value);
  uri_ = source != nullptr ? stringField(*source, "uri") : std::string();
  cacheKey_ = source != nullptr ? stringField(*source, "cacheKey") : std::string();
  sourceChanged_ = true;
}

void ExpoImageView::setContentFit(const folly::dynamic &value) {
  contentFit_ = value.isString() ? value.getString() : "cover";
  fit();
}

void ExpoImageView::setTransition(const folly::dynamic &value) {
  double milliseconds = 0;
  if (value.isNumber()) {
    milliseconds = value.asDouble();
  } else if (value.isObject()) {
    const auto *duration = value.get_ptr("duration");
    if (duration != nullptr && duration->isNumber()) milliseconds = duration->asDouble();
  }
  transitionSeconds_ = std::max(0.0, milliseconds / 1000.0);
}

void ExpoImageView::didUpdateProps() {
  if (!sourceChanged_) return;
  sourceChanged_ = false;
  load();
}

void ExpoImageView::load() {
  const auto request = ++request_;
  cancelDecode();
  evas_object_hide(image_);
  if (uri_.empty()) return;
  dispatchEvent("onLoadStart", folly::dynamic::object());
  runInBackground([alive = std::weak_ptr<ExpoImageView *>(alive_), uri = uri_, cacheKey = cacheKey_, request] {
    runOnMainThread([alive, image = fetchImage(uri, cacheKey), request] {
      auto view = alive.lock();
      if (!view) return;
      if (image.error.empty()) {
        (*view)->decode(image.path, image.cacheType, request);
      } else {
        (*view)->fail(image.error, request);
      }
    });
  });
}

void ExpoImageView::decode(const std::string &path, const std::string &cacheType, unsigned int request) {
  if (request != request_) return;
  cancelDecode();
  evas_object_image_load_size_set(image_, 0, 0);
  evas_object_image_file_set(image_, path.c_str(), nullptr);
  if (evas_object_image_load_error_get(image_) != EVAS_LOAD_ERROR_NONE) {
    fail("Evas cannot decode the image at " + uri_, request);
    return;
  }
  evas_object_image_size_get(image_, &imageWidth_, &imageHeight_);
  const auto drawn = fittedSize();
  if (drawn.w > 0 && drawn.h > 0 && imageWidth_ >= 2 * drawn.w && imageHeight_ >= 2 * drawn.h) {
    evas_object_image_load_size_set(image_, drawn.w, drawn.h);
    evas_object_image_file_set(image_, path.c_str(), nullptr);
  }
  cacheType_ = cacheType;
  decoding_ = true;
  evas_object_image_preload(image_, EINA_FALSE);
}

void ExpoImageView::cancelDecode() {
  if (!decoding_) return;
  decoding_ = false;
  evas_object_image_preload(image_, EINA_TRUE);
}

void ExpoImageView::onPreloaded(void *data, Evas *, Evas_Object *, void *) {
  auto &self = *static_cast<ExpoImageView *>(data);
  if (!self.decoding_) return;
  self.decoding_ = false;
  self.display();
}

void ExpoImageView::display() {
  if (evas_object_image_load_error_get(image_) != EVAS_LOAD_ERROR_NONE) {
    fail("Evas cannot decode the image at " + uri_, request_);
    return;
  }
  fit();
  evas_object_show(image_);
  fadeIn();
  auto source = folly::dynamic::object("url", uri_)("width", imageWidth_)("height", imageHeight_)("mediaType", nullptr)(
      "isAnimated", false);
  dispatchEvent("onLoad", folly::dynamic::object("cacheType", cacheType_)("source", std::move(source)));
  dispatchEvent("onDisplay", folly::dynamic::object());
}

void ExpoImageView::fail(const std::string &message, unsigned int request) {
  if (request != request_) return;
  evas_object_hide(image_);
  dispatchEvent("onError", folly::dynamic::object("error", message));
}

void ExpoImageView::layoutMembers() {
  ExpoView::layoutMembers();
  fit();
}

Evas_Coord_Size ExpoImageView::fittedSize() const {
  const auto &box = canvasRect();
  if (imageWidth_ <= 0 || imageHeight_ <= 0 || box.width <= 0 || box.height <= 0 || contentFit_ == "fill") {
    return {box.width, box.height};
  }
  const double scale = fitScale(
      contentFit_,
      static_cast<double>(box.width) / imageWidth_,
      static_cast<double>(box.height) / imageHeight_,
      pointScale());
  return {
      static_cast<Evas_Coord>(std::lround(imageWidth_ * scale)),
      static_cast<Evas_Coord>(std::lround(imageHeight_ * scale)),
  };
}

void ExpoImageView::fit() {
  const auto &box = canvasRect();
  const auto size = fittedSize();
  setGeometry(
      image_,
      {.x = box.x + (box.width - size.w) / 2, .y = box.y + (box.height - size.h) / 2, .width = size.w, .height = size.h});
}

void ExpoImageView::fadeIn() {
  if (fade_ != nullptr) {
    ecore_animator_del(fade_);
    fade_ = nullptr;
  }
  if (transitionSeconds_ <= 0) {
    evas_object_color_set(image_, 255, 255, 255, 255);
    return;
  }
  evas_object_color_set(image_, 0, 0, 0, 0);
  fade_ = ecore_animator_timeline_add(transitionSeconds_, &ExpoImageView::fadeStep, this);
}

Eina_Bool ExpoImageView::fadeStep(void *data, double position) {
  auto *view = static_cast<ExpoImageView *>(data);
  const auto alpha = static_cast<int>(std::lround(std::clamp(position, 0.0, 1.0) * 255));
  evas_object_color_set(view->image_, alpha, alpha, alpha, alpha);
  if (position >= 1.0) view->fade_ = nullptr;
  return EINA_TRUE;
}

RN_TIZEN_EXPO_VIEW(ViewDefinition<ExpoImageView>("ExpoImage")
                       .prop("source", &ExpoImageView::setSource)
                       .prop("contentFit", &ExpoImageView::setContentFit)
                       .prop("transition", &ExpoImageView::setTransition)
                       .events({"onLoadStart", "onProgress", "onLoad", "onError", "onDisplay"}));

} // namespace rntizen::expo
