#pragma once

#include "PlayerTimer.h"
#include "VideoPlane.h"

#include "platform/AcbLibrary.h"

#include <string>

namespace rnwebos::expo {

/// LG's app-switching control block for this process: made once, under the
/// app's own id, and kept for the life of the process
/// (docs/reference/webos-media-playback.md, section 2.1). Main thread only.
class AcbSession final {
 public:
  AcbSession(const AcbApi &api, const std::string &appId);

  AcbSession(const AcbSession &) = delete;
  AcbSession &operator=(const AcbSession &) = delete;

  const AcbApi &api() const {
    return api_;
  }

  /// Zero when the TV would not make one.
  long handle() const {
    return handle_;
  }

 private:
  const AcbApi &api_;
  long handle_{0};
};

/// The TV's video plane on webOS 4.x, where the compositor has no exported
/// window: the load names a placeholder window, and the control block binds
/// each media the pipeline prepares to the plane, in the order and at the pace
/// the apps that ship on webOS 4 use (section 2.1). The picture is always
/// full screen, whatever rectangle a view asks for, because the rectangle's
/// space differs between webOS 4 chipsets. Main thread only.
class AcbPlane final : public VideoPlane {
 public:
  AcbPlane(AcbSession &session, int player);

  std::string windowId() override;
  bool namesTransport() const override;
  void show(Scene &scene, const PixelRect &source, const PixelRect &destination) override;
  void hide() override;
  void bind(const std::string &mediaId, const VideoStream &video) override;
  void playingChanged(bool playing) override;
  void release() override;

 private:
  void advance(unsigned generation, int step);
  void setState(int playState);
  void showFullScreen();

  AcbSession &session_;
  const int player_;
  unsigned generation_{0};
  std::string mediaId_;
  VideoStream video_;
  bool bound_{false};
  bool playing_{false};
  PlayerTimer pace_;
};

} // namespace rnwebos::expo
