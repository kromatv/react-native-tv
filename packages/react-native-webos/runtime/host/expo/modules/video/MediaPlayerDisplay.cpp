#include "MediaPlayer.h"

namespace rnwebos::expo {

void MediaPlayer::present(const VideoView *owner, const Presentation &presentation) {
  display_.owner = owner;
  display_.presentation = presentation;
  applyDisplay();
}

void MediaPlayer::withdraw(const VideoView *owner) {
  if (owner != display_.owner) return;
  display_.owner = nullptr;
  display_.presentation.reset();
  applyDisplay();
}

void MediaPlayer::applyDisplay() {
  if (!backend_) return;
  const auto &presentation = display_.presentation;
  const auto plane = presentation && presentation->scene != nullptr
                         ? fitVideo(presentation->view, presentation->window, video_.width, video_.height, presentation->fit)
                         : std::nullopt;
  if (!plane) {
    if (std::exchange(display_.plane, std::nullopt)) backend_->plane().hide();
    display_.scene = nullptr;
    return;
  }
  const bool same = display_.plane == plane && display_.scene == presentation->scene &&
                    display_.videoWidth == video_.width && display_.videoHeight == video_.height;
  if (!same) {
    backend_->plane().show(*presentation->scene, sourcePixels(plane->crop, video_.width, video_.height), plane->roi);
  }
  display_.plane = plane;
  display_.scene = presentation->scene;
  display_.videoWidth = video_.width;
  display_.videoHeight = video_.height;
  notifyFirstFrame();
}

bool MediaPlayer::hasDisplay() const {
  return !released_;
}

void MediaPlayer::notifyDisplayChange() {
  if (!display_.presentation || !display_.presentation->onDisplayChange) return;
  const auto callback = display_.presentation->onDisplayChange;
  callback();
}

void MediaPlayer::notifyFirstFrame() {
  const auto *owner = display_.owner;
  if (!playing_ || owner == nullptr || owner == display_.firstFrameOwner || !display_.plane) return;
  display_.firstFrameOwner = owner;
  if (!display_.presentation->onFirstFrame) return;
  const auto callback = display_.presentation->onFirstFrame;
  callback();
}

} // namespace rnwebos::expo
