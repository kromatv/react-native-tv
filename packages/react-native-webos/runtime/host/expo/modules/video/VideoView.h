#pragma once

#include "PlayerModel.h"

#include "../../views/ExpoView.h"

#include <memory>

namespace rnwebos::expo {

class VideoPlayer;

/// expo-video's `VideoView`. The video is on a hardware plane under the
/// app's graphics, so the view only places it and lets it show: its box on
/// screen becomes the plane's destination, fitted by `contentFit`, and it
/// clears its box to transparent, the hole the plane shows through
/// (docs/reference/webos-media-playback.md, section 2). Main thread only.
class VideoView final : public ExpoView {
 public:
  VideoView(Scene &scene, facebook::react::Tag tag);
  ~VideoView() override;

  void setPlayer(const folly::dynamic &value);
  void setContentFit(const folly::dynamic &value);

  void enterFullscreen();
  void exitFullscreen();
  /// Throws: webOS has no picture in picture.
  [[noreturn]] void startPictureInPicture();
  void stopPictureInPicture();

 protected:
  void drawContent(SkCanvas &canvas, float alpha) override;
  int contentLayers() const override;
  void layoutContent() override;
  void didMount() override;
  void willUnmount() override;

 private:
  class FullscreenHole;

  void present();
  void withdraw();
  void detachPlayer();
  void setShown(bool shown);
  PixelRect windowRect() const;

  std::unique_ptr<FullscreenHole> fullscreenHole_;
  std::shared_ptr<VideoPlayer> player_;
  ContentFit fit_{ContentFit::Contain};
  bool fullscreen_{false};
  bool shown_{false};
  PixelRect screen_;
  std::shared_ptr<VideoView *> alive_;
};

} // namespace rnwebos::expo
