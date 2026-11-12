#include "ExpoImageView.h"

#include "ImageCache.h"

#include "../../WorkQueue.h"
#include "../../views/ViewDefinition.h"

#include "threading/MainThread.h"
#include "views/DecodedImages.h"

#include <algorithm>
#include <chrono>
#include <string_view>

namespace rnwebos::expo {

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

ImageFit fitOf(std::string_view contentFit) {
  using enum ImageFit;
  if (contentFit == "contain") return Contain;
  if (contentFit == "fill") return Stretch;
  if (contentFit == "none") return Natural;
  if (contentFit == "scale-down") return Center;
  return Cover;
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

ExpoImageView::ExpoImageView(Scene &scene, facebook::react::Tag tag)
    : ExpoView(scene, tag), alive_(std::make_shared<ExpoImageView *>(this)) {
  setAlwaysClips(true);
}

ExpoImageView::~ExpoImageView() {
  alive_.reset();
  stopFade();
}

void ExpoImageView::setSource(const folly::dynamic &value) {
  const auto *source = firstSource(value);
  uri_ = source != nullptr ? stringField(*source, "uri") : std::string();
  cacheKey_ = source != nullptr ? stringField(*source, "cacheKey") : std::string();
  sourceChanged_ = true;
}

void ExpoImageView::setContentFit(const folly::dynamic &value) {
  content_.setFit(fitOf(value.isString() ? value.getString() : "cover"));
  invalidate();
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
  loadIfNeeded();
}

void ExpoImageView::layoutContent() {
  ExpoView::layoutContent();
  content_.layout(box(), static_cast<float>(pointScale()));
  loadIfNeeded();
}

int ExpoImageView::contentLayers() const {
  return content_.hasImage() ? 1 : 0;
}

void ExpoImageView::drawContent(SkCanvas &canvas, float alpha) {
  content_.draw(canvas, alpha * fadeAlpha_);
}

PixelRect ExpoImageView::box() const {
  const auto size = pixelSize();
  return {.x = 0, .y = 0, .width = size.width, .height = size.height};
}

const std::string &ExpoImageView::memoryKey() const {
  return cacheKey_.empty() ? uri_ : cacheKey_;
}

void ExpoImageView::loadIfNeeded() {
  const auto size = pixelSize();
  if (size.width <= 0 || size.height <= 0) return;
  if (sourceChanged_) {
    sourceChanged_ = false;
    load();
  } else {
    sharpenIfNeeded();
  }
}

void ExpoImageView::load() {
  const auto request = ++request_;
  decode_.reset();
  path_.clear();
  stopFade();
  content_.clear();
  invalidate();
  if (uri_.empty()) return;
  dispatchEvent("onLoadStart", folly::dynamic::object());
  if (const auto kept = findDecodedImage(memoryKey(), content_.fit(), pixelSize(), static_cast<float>(pointScale()))) {
    cacheType_ = "memory";
    display(*kept, false);
    return;
  }
  runInBackground([alive = std::weak_ptr<ExpoImageView *>(alive_), uri = uri_, cacheKey = cacheKey_, request] {
    runOnMainThread([alive, image = fetchImage(uri, cacheKey), request] {
      const auto view = alive.lock();
      if (!view || request != (*view)->request_) return;
      if (!image.error.empty()) {
        (*view)->fail(image.error, request);
        return;
      }
      (*view)->path_ = image.path;
      (*view)->cacheType_ = image.cacheType;
      (*view)->decode((*view)->pixelSize(), false);
    });
  });
}

void ExpoImageView::sharpenIfNeeded() {
  if (!content_.hasImage() || path_.empty() || decode_) return;
  const auto wanted = decodedImageSize(content_.fit(), pixelSize(), content_.size(), static_cast<float>(pointScale()));
  const auto &image = content_.image();
  if (image->width() >= wanted.width && image->height() >= wanted.height) return;
  const auto size = pixelSize();
  decode({.width = 2 * size.width, .height = 2 * size.height}, true);
}

void ExpoImageView::decode(PixelSize box, bool quiet) {
  ImageDecodeRequest wanted{
      .bytes = nullptr, .path = path_, .fit = content_.fit(), .box = box, .naturalScale = static_cast<float>(pointScale())};
  decode_ = decodeImage(std::move(wanted), [this, quiet, request = request_](DecodedImage decoded) {
    decode_.reset();
    if (!decoded.error.empty()) {
      if (!quiet) fail(decoded.error + ": " + uri_, request);
      return;
    }
    display(keepDecodedImage(scene(), memoryKey(), std::move(decoded)), quiet);
  });
}

void ExpoImageView::display(const DecodedImage &decoded, bool quiet) {
  content_.setImage(decoded.image, decoded.size);
  invalidate();
  if (quiet) return;
  fadeIn();
  auto source = folly::dynamic::object("url", uri_)("width", decoded.size.width)("height", decoded.size.height)(
      "mediaType", nullptr)("isAnimated", false);
  dispatchEvent("onLoad", folly::dynamic::object("cacheType", cacheType_)("source", std::move(source)));
  dispatchEvent("onDisplay", folly::dynamic::object());
}

void ExpoImageView::fail(const std::string &message, unsigned int request) {
  if (request != request_) return;
  content_.clear();
  invalidate();
  dispatchEvent("onError", folly::dynamic::object("error", message));
}

void ExpoImageView::fadeIn() {
  stopFade();
  if (transitionSeconds_ <= 0) return;
  fadeAlpha_ = 0;
  fadeStarted_ = MainLoop::Clock::now();
  fade_ = MainLoop::get().addFrameCallback([this](MainLoop::Clock::time_point frameTime) {
    const auto position = std::chrono::duration<double>(frameTime - fadeStarted_).count() / transitionSeconds_;
    fadeAlpha_ = static_cast<float>(std::clamp(position, 0.0, 1.0));
    invalidate();
    if (position >= 1.0) stopFade();
  });
}

void ExpoImageView::stopFade() {
  fadeAlpha_ = 1;
  if (fade_ == 0) return;
  MainLoop::get().removeFrameCallback(fade_);
  fade_ = 0;
}

RN_WEBOS_EXPO_VIEW(ViewDefinition<ExpoImageView>("ExpoImage")
                       .prop("source", &ExpoImageView::setSource)
                       .prop("contentFit", &ExpoImageView::setContentFit)
                       .prop("transition", &ExpoImageView::setTransition)
                       .events({"onLoadStart", "onProgress", "onLoad", "onError", "onDisplay"}));

} // namespace rnwebos::expo
