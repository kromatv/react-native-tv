#pragma once

#include "VideoPlane.h"

#include "modules/skparagraph/include/Paragraph.h"

#include <memory>
#include <string>

namespace rnwebos::expo {

/// The video plane where there is none: a dark test picture with the player's
/// state written on it, composited under everything the scene drew, as the
/// TV composites its plane under the app's graphics. It shows only where the
/// app left its pixels transparent, so a screenshot tells where the video is,
/// how large, and whether the hole is there. Main thread only.
class SimulatedPlane final : public VideoPlane, private SceneLayer {
 public:
  SimulatedPlane() = default;
  ~SimulatedPlane() override;

  std::string windowId() override;
  void show(Scene &scene, const PixelRect &source, const PixelRect &destination) override;
  void hide() override;

  /// What the picture is: a frame of `frame` pixels, `caption` at its top
  /// left and `subtitle`, when not empty, at its bottom.
  void setPicture(PixelSize frame, std::string caption, std::string subtitle);

  bool isShown() const;
  const PixelRect &source() const;
  const PixelRect &destination() const;

 private:
  void draw(SkCanvas &canvas) override;
  void drawFrame(SkCanvas &canvas, const SkRect &target) const;
  void invalidate();

  Scene *scene_{nullptr};
  PixelRect source_;
  PixelRect destination_;
  PixelSize frame_;
  std::string caption_;
  std::string subtitle_;
  std::unique_ptr<skia::textlayout::Paragraph> captionText_;
  std::unique_ptr<skia::textlayout::Paragraph> subtitleText_;
};

} // namespace rnwebos::expo
