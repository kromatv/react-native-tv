#include "SimulatedPlane.h"

#include "render/OverlayText.h"

#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"

#include <algorithm>
#include <utility>

namespace rnwebos::expo {

namespace {

constexpr int kPlaneOrder = Scene::kErrorOrder + 1000;
constexpr SkColor kPicture = SkColorSetRGB(0x16, 0x1d, 0x2e);
constexpr SkColor kPattern = SkColorSetRGB(0x3b, 0x4a, 0x6b);
constexpr SkColor kSubtitle = SkColorSetRGB(0xff, 0xd2, 0x3f);
constexpr float kFullTextHeight = 600;

SkRect rectOf(const PixelRect &rect) {
  return SkRect::MakeXYWH(
      static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.width), static_cast<float>(rect.height));
}

} // namespace

SimulatedPlane::~SimulatedPlane() {
  hide();
}

std::string SimulatedPlane::windowId() {
  return "_Window_Id_simulated";
}

void SimulatedPlane::show(Scene &scene, const PixelRect &source, const PixelRect &destination) {
  if (scene_ != &scene) {
    hide();
    scene_ = &scene;
    scene.addLayer(*this, kPlaneOrder);
  }
  if (source == source_ && destination == destination_) return;
  if (destination.width != destination_.width || destination.height != destination_.height) {
    captionText_.reset();
    subtitleText_.reset();
  }
  source_ = source;
  destination_ = destination;
  invalidate();
}

void SimulatedPlane::hide() {
  if (scene_ == nullptr) return;
  std::exchange(scene_, nullptr)->removeLayer(*this);
  source_ = {};
  destination_ = {};
}

void SimulatedPlane::setPicture(PixelSize frame, std::string caption, std::string subtitle) {
  if (frame == frame_ && caption == caption_ && subtitle == subtitle_) return;
  frame_ = frame;
  if (caption != caption_) captionText_.reset();
  if (subtitle != subtitle_) subtitleText_.reset();
  caption_ = std::move(caption);
  subtitle_ = std::move(subtitle);
  invalidate();
}

void SimulatedPlane::invalidate() {
  if (scene_ != nullptr) scene_->invalidate();
}

// The decoded frame's outline, diagonals and a circle, in frame pixels, so the
// source and destination rectangles crop and stretch them as a plane would.
void SimulatedPlane::drawFrame(SkCanvas &canvas, const SkRect &target) const {
  const auto frame = frame_.width > 0 && frame_.height > 0
                         ? SkRect::MakeWH(static_cast<float>(frame_.width), static_cast<float>(frame_.height))
                         : SkRect::MakeWH(target.width(), target.height());
  const auto source = source_.width > 0 && source_.height > 0 ? rectOf(source_) : frame;
  SkAutoCanvasRestore restore(&canvas, true);
  canvas.concat(SkMatrix::RectToRect(source, target));
  SkPaint line;
  line.setColor(kPattern);
  line.setAntiAlias(true);
  line.setStyle(SkPaint::kStroke_Style);
  line.setStrokeWidth(std::max(2.0f, frame.height() / 180));
  canvas.drawRect(frame.makeInset(line.getStrokeWidth() / 2, line.getStrokeWidth() / 2), line);
  canvas.drawLine(frame.left(), frame.top(), frame.right(), frame.bottom(), line);
  canvas.drawLine(frame.right(), frame.top(), frame.left(), frame.bottom(), line);
  canvas.drawCircle(frame.centerX(), frame.centerY(), frame.height() * 0.45f, line);
}

void SimulatedPlane::draw(SkCanvas &canvas) {
  if (scene_ == nullptr || destination_.width <= 0 || destination_.height <= 0) return;
  const auto target = rectOf(destination_);
  const auto scale =
      static_cast<float>(scene_->size().height) / 1080 * std::clamp(target.height() / kFullTextHeight, 0.5f, 1.0f);
  const float margin = 20 * scale;
  const float textWidth = std::max(1.0f, target.width() - 2 * margin);
  if (!captionText_) captionText_ = overlayParagraph({{.text = caption_, .size = 22 * scale}}, textWidth);
  if (!subtitleText_ && !subtitle_.empty()) {
    subtitleText_ = overlayParagraph({{.text = subtitle_, .size = 34 * scale, .color = kSubtitle, .bold = true}}, textWidth);
  }
  SkPaint under;
  under.setBlendMode(SkBlendMode::kDstOver);
  SkAutoCanvasRestore restore(&canvas, false);
  canvas.saveLayer(&target, &under);
  canvas.clipRect(target);
  canvas.drawColor(kPicture);
  drawFrame(canvas, target);
  captionText_->paint(&canvas, target.left() + margin, target.top() + margin);
  if (subtitleText_ && !subtitle_.empty()) {
    subtitleText_->paint(&canvas, target.left() + margin, target.bottom() - margin - subtitleText_->getHeight());
  }
}

} // namespace rnwebos::expo
