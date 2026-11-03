#include "VideoView.h"

#include "SubtitleOverlay.h"
#include "VideoPlayer.h"
#include "VideoWindow.h"

#include "../../SharedObjectRegistry.h"
#include "../../views/ViewDefinition.h"

#include "logging/Log.h"

#include <stdexcept>

namespace rntizen::expo {

namespace {

constexpr int kFullscreenLayer = 2000;

Evas_Object *transparentHole(Evas *evas) {
  auto *hole = evas_object_rectangle_add(evas);
  evas_object_color_set(hole, 0, 0, 0, 0);
  evas_object_render_op_set(hole, EVAS_RENDER_COPY);
  return hole;
}

} // namespace

VideoView::VideoView(Evas *evas, facebook::react::Tag tag)
    : ExpoView(evas, tag),
      hole_(transparentHole(evas)),
      subtitle_(std::make_unique<SubtitleOverlay>(evas)),
      alive_(std::make_shared<VideoView *>(this)) {
  addMember(hole_, true);
  evas_object_hide(hole_);
  addMember(subtitle_->object(), false);
  subtitle_->clear();
}

VideoView::~VideoView() {
  followScreenRect(false);
  alive_.reset();
  detachPlayer();
  subtitle_.reset();
  if (topHole_ != nullptr) evas_object_del(topHole_);
  evas_object_del(hole_);
}

void VideoView::setPlayer(const folly::dynamic &value) {
  detachPlayer();
  if (value.isNumber()) {
    player_ = SharedObjectRegistry::shared().find<VideoPlayer>(static_cast<SharedObjectId>(value.asDouble()));
  }
  log(LogLevel::Info,
      "VideoView " + std::to_string(tag()) +
          (player_ ? " shows video " + std::to_string(player_->number()) : std::string(" has no player")));
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
  throw std::runtime_error("Picture in picture is not supported on Tizen");
}

void VideoView::stopPictureInPicture() {}

void VideoView::layoutMembers() {
  ExpoView::layoutMembers();
  present();
}

PixelRect VideoView::windowRect() const {
  PixelRect window;
  evas_output_size_get(evas(), &window.width, &window.height);
  return window;
}

void VideoView::present() {
  const auto &box = canvasRect();
  setGeometry(hole_, box);
  subtitle_->place(fullscreen_ ? windowRect() : box, pointScale());
  const std::shared_ptr<TizenPlayer> tizen = player_ ? player_->tizenPlayer() : nullptr;
  const bool showing = tizen != nullptr && tizen->hasDisplay() && box.width > 0 && box.height > 0;
  setShown(showing);
  screen_ = screenRect();
  showHoles(showing, screen_ != box);
  if (!tizen) return;
  const auto window = windowRect();
  const std::weak_ptr<VideoView *> alive = alive_;
  tizen->present(
      this,
      Presentation{
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
          .onSubtitle =
              [alive](const std::string &text, unsigned long milliseconds) {
                if (auto view = alive.lock()) (*view)->subtitle_->show(text, milliseconds);
              },
      });
}

void VideoView::showHoles(bool shown, bool mapped) {
  if (shown && !fullscreen_ && !mapped) {
    evas_object_show(hole_);
  } else {
    evas_object_hide(hole_);
  }
  const bool onTop = shown && (fullscreen_ || mapped);
  if (onTop && topHole_ == nullptr) topHole_ = transparentHole(evas());
  if (topHole_ == nullptr) return;
  if (!onTop) {
    evas_object_hide(topHole_);
    return;
  }
  evas_object_layer_set(topHole_, fullscreen_ ? kFullscreenLayer : 0);
  setGeometry(topHole_, fullscreen_ ? windowRect() : screen_);
  evas_object_raise(topHole_);
  evas_object_show(topHole_);
}

void VideoView::detachPlayer() {
  if (player_ && player_->tizenPlayer()) player_->tizenPlayer()->withdraw(this);
  if (subtitle_) subtitle_->clear();
  player_.reset();
  setShown(false);
}

void VideoView::setShown(bool shown) {
  if (shown == shown_) return;
  shown_ = shown;
  followScreenRect(shown);
  videoViewShown(shown);
}

void VideoView::followScreenRect(bool follow) {
  if (!follow) {
    if (screenTracker_ != nullptr) ecore_animator_del(screenTracker_);
    screenTracker_ = nullptr;
    return;
  }
  if (screenTracker_ != nullptr) return;
  screenTracker_ = ecore_animator_add(
      [](void *data) -> Eina_Bool {
        auto *view = static_cast<VideoView *>(data);
        if (view->screenRect() != view->screen_) view->present();
        return ECORE_CALLBACK_RENEW;
      },
      this);
}

RN_TIZEN_EXPO_VIEW(ViewDefinition<VideoView>("ExpoVideo", "VideoView")
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

} // namespace rntizen::expo
