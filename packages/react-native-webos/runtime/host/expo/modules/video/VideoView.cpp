#include "VideoView.h"

#include "VideoPlayer.h"
#include "VideoWindow.h"

#include "../../SharedObjectRegistry.h"
#include "../../views/ViewDefinition.h"

#include "logging/Log.h"

#include "include/core/SkPaint.h"

#include <format>
#include <stdexcept>

namespace rnwebos::expo {

namespace {

constexpr int kFullscreenOrder = Scene::kDialogOrder - 1;

class UnsupportedFeatureError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

} // namespace

/// Clears the whole window above every view while the video is fullscreen.
class VideoView::FullscreenHole final : public SceneLayer {
 public:
  explicit FullscreenHole(Scene &scene) : scene_(scene) {
    scene_.addLayer(*this, kFullscreenOrder);
  }

  ~FullscreenHole() override {
    scene_.removeLayer(*this);
  }

  void draw(SkCanvas &canvas) override {
    canvas.clear(SK_ColorTRANSPARENT);
  }

 private:
  Scene &scene_;
};

VideoView::VideoView(Scene &scene, facebook::react::Tag tag)
    : ExpoView(scene, tag), alive_(std::make_shared<VideoView *>(this)) {}

VideoView::~VideoView() {
  alive_.reset();
  detachPlayer();
}

void VideoView::setPlayer(const folly::dynamic &value) {
  detachPlayer();
  if (value.isNumber()) {
    player_ = SharedObjectRegistry::shared().find<VideoPlayer>(static_cast<SharedObjectId>(value.asDouble()));
  }
  const auto shows = player_ ? std::format("shows video {}", player_->number()) : std::string("has no player");
  log(LogLevel::Info, std::format("VideoView {} {}", tag(), shows));
  present();
}

void VideoView::setContentFit(const folly::dynamic &value) {
  fit_ = contentFitFrom(value.isString() ? value.getString() : "contain");
  present();
}

void VideoView::enterFullscreen() {
  if (fullscreen_) return;
  fullscreen_ = true;
  dispatchEvent("onFullscreenEnter", folly::dynamic::object());
  present();
}

void VideoView::exitFullscreen() {
  if (!fullscreen_) return;
  fullscreen_ = false;
  dispatchEvent("onFullscreenExit", folly::dynamic::object());
  present();
}

void VideoView::startPictureInPicture() {
  throw UnsupportedFeatureError("Picture in picture is not supported on webOS");
}

void VideoView::stopPictureInPicture() {
  // Never started: webOS has no picture in picture.
}

void VideoView::layoutContent() {
  present();
}

void VideoView::didMount() {
  present();
}

void VideoView::willUnmount() {
  withdraw();
}

int VideoView::contentLayers() const {
  return 1;
}

// An ancestor that scrolls or animates moves the view without telling it: the
// plane is placed again in the first frame drawn after the box moved on screen.
void VideoView::drawContent(SkCanvas &canvas, float) {
  if (!shown_ || fullscreen_) return;
  const auto size = pixelSize();
  SkPaint hole;
  hole.setBlendMode(SkBlendMode::kClear);
  canvas.drawRect(SkRect::MakeWH(static_cast<float>(size.width), static_cast<float>(size.height)), hole);
  if (screenRect() != screen_) present();
}

PixelRect VideoView::windowRect() const {
  const auto size = scene().size();
  return {.x = 0, .y = 0, .width = size.width, .height = size.height};
}

void VideoView::present() {
  const auto media = player_ ? player_->mediaPlayer() : nullptr;
  screen_ = screenRect();
  const bool showing =
      parent() != nullptr && media != nullptr && media->hasDisplay() && screen_.width > 0 && screen_.height > 0;
  setShown(showing);
  if (showing && fullscreen_) {
    if (!fullscreenHole_) fullscreenHole_ = std::make_unique<FullscreenHole>(scene());
  } else {
    fullscreenHole_.reset();
  }
  invalidate();
  if (!media) return;
  if (!showing) {
    media->withdraw(this);
    return;
  }
  const auto window = windowRect();
  const std::weak_ptr<VideoView *> alive = alive_;
  media->present(
      this,
      Presentation{
          .scene = &scene(),
          .view = fullscreen_ ? window : screen_,
          .window = window,
          .fit = fit_,
          .onFirstFrame =
              [alive] {
                if (auto view = alive.lock()) (*view)->dispatchEvent("onFirstFrameRender", folly::dynamic::object());
              },
          .onDisplayChange =
              [alive] {
                if (auto view = alive.lock()) (*view)->present();
              },
      });
}

void VideoView::withdraw() {
  if (player_ && player_->mediaPlayer()) player_->mediaPlayer()->withdraw(this);
  fullscreenHole_.reset();
  setShown(false);
}

void VideoView::detachPlayer() {
  withdraw();
  player_.reset();
}

void VideoView::setShown(bool shown) {
  if (shown == shown_) return;
  shown_ = shown;
  videoViewShown(scene(), shown);
}

RN_WEBOS_EXPO_VIEW(ViewDefinition<VideoView>("ExpoVideo", "VideoView")
                       .prop("player", &VideoView::setPlayer)
                       .prop("contentFit", &VideoView::setContentFit)
                       .asyncFunction("enterFullscreen", &VideoView::enterFullscreen)
                       .asyncFunction("exitFullscreen", &VideoView::exitFullscreen)
                       .asyncFunction("startPictureInPicture", &VideoView::startPictureInPicture)
                       .asyncFunction("stopPictureInPicture", &VideoView::stopPictureInPicture)
                       .events(
                           {"onFullscreenEnter",
                            "onFullscreenExit",
                            "onFirstFrameRender",
                            "onPictureInPictureStart",
                            "onPictureInPictureStop"}));

} // namespace rnwebos::expo
