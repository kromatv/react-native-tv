#pragma once

#include "VideoPlane.h"

namespace rnwebos::expo {

/// The TV's video plane, placed through the "exported window" the compositor
/// gives the app's surface on webOS 5 and later: SDL's webOS calls make it,
/// its id binds the pipeline's output to it, and its source and destination
/// rectangles place the picture (docs/reference/webos-media-playback.md,
/// section 2). Off a TV it has no window and no id. Main thread only.
class ExportedWindowPlane final : public VideoPlane {
 public:
  explicit ExportedWindowPlane(int player);
  ~ExportedWindowPlane() override;

  std::string windowId() override;
  void show(Scene &scene, const PixelRect &source, const PixelRect &destination) override;
  void hide() override;

 private:
  const int player_;
  std::string windowId_;
  bool created_{false};
  bool moved_{false};
  PixelRect source_;
  PixelRect destination_;
};

} // namespace rnwebos::expo
