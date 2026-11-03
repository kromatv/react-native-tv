#include "TizenPlayer.h"

#include "PlayerLog.h"

#include <format>

namespace rntizen::expo {

void TizenPlayer::present(const VideoView *owner, const Presentation &presentation) {
  display_.owner = owner;
  display_.presentation = presentation;
  applyDisplay();
}

void TizenPlayer::withdraw(const VideoView *owner) {
  if (owner != display_.owner) return;
  display_.owner = nullptr;
  display_.presentation.reset();
  applyDisplay();
}

void TizenPlayer::applyDisplay() {
  if (handle_ == nullptr || !display_.bound) return;
  const auto &presentation = display_.presentation;
  const auto plane =
      presentation ? fitVideo(presentation->view, presentation->window, video_.width, video_.height, presentation->fit)
                   : std::nullopt;
  if (!plane) {
    setVisible(false);
    return;
  }
  if (!display_.plane || display_.plane->roi != plane->roi) {
    const auto &roi = plane->roi;
    const int placed = player_set_display_roi_area(handle_, roi.x, roi.y, roi.width, roi.height);
    playerLog(
        number_, std::format("ROI {},{} {}x{} {}", roi.x, roi.y, roi.width, roi.height, playerErrorName(placed)));
  }
  if (!display_.plane || display_.plane->crop != plane->crop) {
    const auto &crop = plane->crop;
    const int cropped = player_set_video_roi_area(handle_, crop.x, crop.y, crop.width, crop.height);
    playerLog(
        number_,
        std::format(
            "source crop {},{} {}x{} {}",
            decimal(crop.x),
            decimal(crop.y),
            decimal(crop.width),
            decimal(crop.height),
            playerErrorName(cropped)));
  }
  display_.plane = plane;
  setVisible(true);
  notifyFirstFrame();
}

bool TizenPlayer::hasDisplay() const {
  return display_.bound;
}

void TizenPlayer::notifyDisplayChange() {
  if (!display_.presentation || !display_.presentation->onDisplayChange) return;
  const auto callback = display_.presentation->onDisplayChange;
  callback();
}

void TizenPlayer::notifyFirstFrame() {
  const auto *owner = display_.owner;
  if (!playing_ || owner == nullptr || owner == display_.firstFrameOwner || !display_.presentation) return;
  display_.firstFrameOwner = owner;
  if (!display_.presentation->onFirstFrame) return;
  const auto callback = display_.presentation->onFirstFrame;
  callback();
}

void TizenPlayer::setVisible(bool visible) {
  if (handle_ == nullptr || visible == display_.visible) return;
  display_.visible = visible;
  playerLog(
      number_,
      std::format(
          "display {} {}",
          visible ? "visible" : "hidden",
          playerErrorName(player_set_display_visible(handle_, visible))));
}

} // namespace rntizen::expo
