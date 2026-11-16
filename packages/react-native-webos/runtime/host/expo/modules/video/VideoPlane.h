#pragma once

#include "PlayerModel.h"

#include "render/Scene.h"
#include "views/PixelGeometry.h"

#include <string>

namespace rnwebos::expo {

/// Where the picture itself shows: a hardware plane under the app's graphics,
/// seen through the transparent hole a VideoView leaves
/// (docs/reference/webos-media-playback.md, section 2). Main thread only.
class VideoPlane {
 public:
  virtual ~VideoPlane() = default;

  /// The id the media pipeline binds its output to, `option.windowId` of a
  /// load; empty when the plane has none to give.
  virtual std::string windowId() = 0;

  /// Shows the part `source` of the decoded frame, in frame pixels, at
  /// `destination`, in the window pixels of `scene`. An empty `source` is the
  /// whole frame, for as long as its size is unknown.
  virtual void show(Scene &scene, const PixelRect &source, const PixelRect &destination) = 0;

  /// The plane has no view to show in any more.
  virtual void hide() = 0;

  /// Whether a load may name its transport, `mediaTransportType`: a webOS 4
  /// pipeline answers such a load and never fetches.
  virtual bool namesTransport() const {
    return true;
  }

  /// The pipeline prepared `mediaId`, whose picture is `video`: a plane that
  /// has to be told which media it shows is told now.
  virtual void bind(const std::string & /*mediaId*/, const VideoStream & /*video*/) {
    // A plane the window id binds needs nothing more.
  }

  /// The media this plane shows started or stopped playing.
  virtual void playingChanged(bool /*playing*/) {
    // A plane the window id binds follows the pipeline by itself.
  }

  /// The media this plane showed is unloaded.
  virtual void release() {
    // A plane the window id binds is freed with the media.
  }
};

} // namespace rnwebos::expo
