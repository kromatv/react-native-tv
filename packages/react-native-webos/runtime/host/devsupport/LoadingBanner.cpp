#include "LoadingBanner.h"

#include "render/OverlayText.h"

#include "include/core/SkPaint.h"

namespace rnwebos {

namespace {

constexpr int kHeightDivisor = 18;
constexpr int kFontDivisor = 40;
// Wide enough for any message: the banner's text never wraps, it is clipped.
constexpr float kOneLine = 100000;

} // namespace

LoadingBanner::LoadingBanner(Scene &scene) : scene_(scene) {}

LoadingBanner::~LoadingBanner() {
  if (text_) scene_.removeLayer(*this);
}

void LoadingBanner::show(const std::string &message, std::uint32_t textColor, std::uint32_t backgroundColor) {
  const bool wasShown = isShown();
  const auto fontSize = static_cast<float>(scene_.size().height / kFontDivisor);
  text_ = overlayParagraph({{.text = message, .size = fontSize, .color = textColor}}, kOneLine);
  backgroundColor_ = backgroundColor;
  if (!wasShown) scene_.addLayer(*this, Scene::kBannerOrder);
  scene_.invalidate();
}

void LoadingBanner::hide() {
  if (!isShown()) return;
  text_.reset();
  scene_.removeLayer(*this);
}

bool LoadingBanner::isShown() const {
  return text_ != nullptr;
}

void LoadingBanner::draw(SkCanvas &canvas) {
  if (!text_) return;
  const auto size = scene_.size();
  const auto strip = SkRect::MakeWH(static_cast<float>(size.width), static_cast<float>(size.height / kHeightDivisor));
  SkPaint background;
  background.setColor(backgroundColor_);
  SkAutoCanvasRestore restore(&canvas, true);
  canvas.clipRect(strip);
  canvas.drawRect(strip, background);
  text_->paint(&canvas, strip.centerX() - text_->getLongestLine() / 2, strip.centerY() - text_->getHeight() / 2);
}

} // namespace rnwebos
