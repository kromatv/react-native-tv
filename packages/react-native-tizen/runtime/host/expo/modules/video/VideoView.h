#pragma once

#include "PlayerModel.h"

#include "../../views/ExpoView.h"

#include <Ecore.h>

#include <memory>

namespace rntizen::expo {

class SubtitleOverlay;
class TizenPlayer;
class VideoPlayer;
struct VideoHoles;

/// expo-video's `VideoView`. The video is on the overlay plane under the
/// window, so the view only places it: its box in window pixels becomes the
/// player's ROI, fitted by `contentFit`, and a hole over the box lets the
/// plane show. The hole is opaque black until the plane has a picture.
/// Main thread only.
class VideoView final : public ExpoView {
 public:
  VideoView(Evas *evas, facebook::react::Tag tag);
  ~VideoView() override;

  void setPlayer(const folly::dynamic &value);
  void setContentFit(const folly::dynamic &value);

  void enterFullscreen();
  void exitFullscreen();
  /// Throws: Tizen has no picture in picture.
  [[noreturn]] void startPictureInPicture();
  void stopPictureInPicture();

 protected:
  void layoutMembers() override;

 private:
  void present();
  void presentOn(TizenPlayer &tizen);
  void showHoles(const VideoHoles &holes);
  void detachPlayer();
  void setShown(bool shown);
  void followScreenRect(bool follow);
  PixelRect windowRect() const;

  Evas_Object *const hole_;
  Evas_Object *topHole_{nullptr};
  std::unique_ptr<SubtitleOverlay> subtitle_;
  std::shared_ptr<VideoPlayer> player_;
  ContentFit fit_{ContentFit::Contain};
  bool fullscreen_{false};
  bool shown_{false};
  Ecore_Animator *screenTracker_{nullptr};
  PixelRect screen_;
  std::shared_ptr<VideoView *> alive_;
};

} // namespace rntizen::expo
