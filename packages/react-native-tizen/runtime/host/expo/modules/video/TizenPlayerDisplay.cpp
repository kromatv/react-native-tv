#include "TizenPlayer.h"

#include "PlayerLog.h"

namespace rntizen::expo {

void TizenPlayer::present(const void *owner, const Presentation &presentation) {
  owner_ = owner;
  presentation_ = presentation;
  applyDisplay();
}

void TizenPlayer::withdraw(const void *owner) {
  if (owner != owner_) return;
  owner_ = nullptr;
  presentation_.reset();
  applyDisplay();
}

void TizenPlayer::applyDisplay() {
  if (handle_ == nullptr || !displayBound_) return;
  const auto plane = presentation_ ? fitVideo(presentation_->view, presentation_->window, video_.width, video_.height,
                                              presentation_->fit)
                                   : std::nullopt;
  if (!plane) {
    setVisible(false);
    return;
  }
  if (!plane_ || plane_->roi != plane->roi) {
    const auto &roi = plane->roi;
    const int placed = player_set_display_roi_area(handle_, roi.x, roi.y, roi.width, roi.height);
    playerLog(number_, "ROI " + std::to_string(roi.x) + "," + std::to_string(roi.y) + " " + std::to_string(roi.width) +
                           "x" + std::to_string(roi.height) + " " + playerErrorName(placed));
  }
  if (!plane_ || plane_->crop != plane->crop) {
    const auto &crop = plane->crop;
    const int cropped = player_set_video_roi_area(handle_, crop.x, crop.y, crop.width, crop.height);
    playerLog(number_, "source crop " + decimal(crop.x) + "," + decimal(crop.y) + " " + decimal(crop.width) + "x" +
                           decimal(crop.height) + " " +
                           playerErrorName(cropped));
  }
  plane_ = plane;
  setVisible(true);
  notifyFirstFrame();
}

bool TizenPlayer::hasDisplay() const {
  return displayBound_;
}

void TizenPlayer::notifyDisplayChange() {
  if (!presentation_ || !presentation_->onDisplayChange) return;
  const auto callback = presentation_->onDisplayChange;
  callback();
}

void TizenPlayer::notifyFirstFrame() {
  if (!playing_ || owner_ == nullptr || owner_ == firstFrameOwner_ || !presentation_) return;
  firstFrameOwner_ = owner_;
  if (!presentation_->onFirstFrame) return;
  const auto callback = presentation_->onFirstFrame;
  callback();
}

void TizenPlayer::setVisible(bool visible) {
  if (handle_ == nullptr || visible == visible_) return;
  visible_ = visible;
  playerLog(number_, std::string("display ") + (visible ? "visible " : "hidden ") +
                         playerErrorName(player_set_display_visible(handle_, visible)));
}

} // namespace rntizen::expo
