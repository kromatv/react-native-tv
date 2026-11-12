#pragma once

#include "render/Scene.h"

#include "include/core/SkColor.h"
#include "modules/skparagraph/include/Paragraph.h"

#include <cstdint>
#include <memory>
#include <string>

namespace rnwebos {

/// The strip across the top of the screen that says "Loading from ..." or
/// "Refreshing...", which Android draws for `DevLoadingView`. Colors are
/// 0xAARRGGBB, as `processColor` gives them. Main thread only.
class LoadingBanner final : public SceneLayer {
 public:
  explicit LoadingBanner(Scene &scene);
  ~LoadingBanner() override;

  LoadingBanner(const LoadingBanner &) = delete;
  LoadingBanner &operator=(const LoadingBanner &) = delete;

  void show(const std::string &message, std::uint32_t textColor, std::uint32_t backgroundColor);
  void hide();
  bool isShown() const;

  void draw(SkCanvas &canvas) override;

 private:
  Scene &scene_;
  SkColor backgroundColor_{SK_ColorTRANSPARENT};
  std::unique_ptr<skia::textlayout::Paragraph> text_;
};

} // namespace rnwebos
