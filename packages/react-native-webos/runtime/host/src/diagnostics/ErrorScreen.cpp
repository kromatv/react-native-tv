#include "ErrorScreen.h"

#include "../render/OverlayText.h"

namespace rnwebos {

namespace {

constexpr float kMargin = 96;
constexpr std::size_t kMaxStackLines = 18;
constexpr SkColor kBackground = SkColorSetRGB(0x9b, 0x1c, 0x1c);
constexpr SkColor kDim = SkColorSetRGB(0xff, 0xd9, 0xd9);

std::vector<OverlayRun> runsFor(const ErrorReport &report, float scale) {
  std::vector<OverlayRun> runs{
      {.text = report.title + "\n\n", .size = 48 * scale, .bold = true},
      {.text = report.message + "\n", .size = 28 * scale},
  };
  if (!report.stack.empty()) runs.push_back({.text = "\n", .size = 22 * scale});
  for (std::size_t index = 0; index < report.stack.size() && index < kMaxStackLines; ++index) {
    runs.push_back({.text = report.stack[index] + "\n", .size = 22 * scale, .color = kDim, .monospace = true});
  }
  runs.push_back({
      .text = "\nThe full report is in rn-webos logs. Press Back to close the app.",
      .size = 24 * scale,
      .color = kDim,
  });
  return runs;
}

} // namespace

ErrorScreen::ErrorScreen(Scene &scene) : scene_(scene) {}

ErrorScreen::~ErrorScreen() {
  if (text_) scene_.removeLayer(*this);
}

void ErrorScreen::show(const ErrorReport &report) {
  const auto size = scene_.size();
  const auto scale = static_cast<float>(size.height) / 1080;
  const bool wasShown = isShown();
  text_ = overlayParagraph(runsFor(report, scale), static_cast<float>(size.width) - 2 * kMargin * scale);
  if (!wasShown) scene_.addLayer(*this, Scene::kErrorOrder);
  scene_.invalidate();
}

bool ErrorScreen::isShown() const {
  return text_ != nullptr;
}

void ErrorScreen::draw(SkCanvas &canvas) {
  if (!text_) return;
  const auto scale = static_cast<float>(scene_.size().height) / 1080;
  canvas.drawColor(kBackground);
  text_->paint(&canvas, kMargin * scale, kMargin * scale);
}

} // namespace rnwebos
