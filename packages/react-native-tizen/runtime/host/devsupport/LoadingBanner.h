#pragma once

#include <Evas.h>

#include <cstdint>
#include <string>

namespace rntizen {

/// The strip across the top of the screen that says "Loading from ..." or
/// "Refreshing...", which Android draws for `DevLoadingView`. Colors are
/// 0xAARRGGBB, as `processColor` gives them. Main thread only.
class LoadingBanner final {
 public:
  LoadingBanner(Evas *evas, int screenWidth, int screenHeight);
  ~LoadingBanner();

  LoadingBanner(const LoadingBanner &) = delete;
  LoadingBanner &operator=(const LoadingBanner &) = delete;

  void show(const std::string &message, std::uint32_t textColor, std::uint32_t backgroundColor);
  void hide();

 private:
  const int fontSize_;
  Evas_Object *background_;
  Evas_Object *text_;
  Evas_Textblock_Style *style_;
};

} // namespace rntizen
