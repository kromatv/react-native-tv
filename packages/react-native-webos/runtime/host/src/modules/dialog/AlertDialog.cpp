#include "AlertDialog.h"

#include "../../render/OverlayText.h"

#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"

#include <algorithm>
#include <utility>

namespace rnwebos {

namespace {

constexpr float kLogicalHeight = 1080;
constexpr float kPanelWidth = 1040;
constexpr float kPanelRadius = 16;
constexpr float kPadding = 56;
constexpr float kButtonWidth = 260;
constexpr float kButtonHeight = 76;
constexpr float kButtonRadius = 10;
constexpr float kButtonGap = 24;
constexpr float kTextToButtons = 48;
constexpr float kTitleSize = 42;
constexpr float kTextSize = 30;
// Wide enough for any label: a button's text never wraps, it is clipped.
constexpr float kOneLine = 100000;
constexpr SkColor kScrim = SkColorSetARGB(170, 0, 0, 0);
constexpr SkColor kPanel = SkColorSetRGB(0x1f, 0x24, 0x33);
constexpr SkColor kText = SkColorSetRGB(0xd7, 0xde, 0xf0);
constexpr SkColor kButton = SkColorSetRGB(0x2d, 0x34, 0x47);
constexpr SkColor kFocusedButton = SkColorSetRGB(0x3d, 0x5a, 0xfe);

SkPaint fill(SkColor color) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(color);
  return paint;
}

} // namespace

AlertDialog::AlertDialog(Scene &scene) : scene_(scene) {}

AlertDialog::~AlertDialog() {
  if (isLayer_) scene_.removeLayer(*this);
}

void AlertDialog::show(AlertRequest request) {
  pending_.push_back(std::move(request));
  if (!current_) present();
}

bool AlertDialog::isShown() const {
  return current_.has_value();
}

void AlertDialog::handle(const RemoteEvent &event) {
  if (!current_ || event.keyAction != 1) return;
  if (event.isBack) {
    if (current_->cancelable) close(std::nullopt);
  } else if (event.eventType == "left" && focused_ > 0) {
    focus(focused_ - 1);
  } else if (event.eventType == "right" && focused_ + 1 < current_->buttons.size()) {
    focus(focused_ + 1);
  } else if (event.eventType == "select" && focused_ < current_->buttons.size()) {
    close(current_->buttons[focused_].key);
  }
}

void AlertDialog::present() {
  if (pending_.empty()) return;
  current_ = std::move(pending_.front());
  pending_.pop_front();
  layout();
  focused_ = current_->buttons.empty() ? 0 : current_->buttons.size() - 1;
  if (!std::exchange(isLayer_, true)) scene_.addLayer(*this, Scene::kDialogOrder);
  scene_.invalidate();
}

void AlertDialog::layout() {
  const auto screen = scene_.size();
  const auto scale = static_cast<float>(screen.height) / kLogicalHeight;
  std::vector<OverlayRun> runs{{.text = current_->title, .size = kTitleSize * scale, .bold = true}};
  if (!current_->message.empty()) {
    runs.push_back({.text = "\n\n" + current_->message, .size = kTextSize * scale, .color = kText});
  }
  text_ = overlayParagraph(runs, (kPanelWidth - 2 * kPadding) * scale);
  const auto count = static_cast<float>(current_->buttons.size());
  const auto buttonsHeight = current_->buttons.empty() ? 0 : (kTextToButtons + kButtonHeight) * scale;
  const auto panelHeight =
      std::min(static_cast<float>(screen.height), 2 * kPadding * scale + text_->getHeight() + buttonsHeight);
  panel_ = SkRect::MakeXYWH(
      (static_cast<float>(screen.width) - kPanelWidth * scale) / 2,
      (static_cast<float>(screen.height) - panelHeight) / 2,
      kPanelWidth * scale,
      panelHeight);
  const auto rowWidth = (count * kButtonWidth + std::max(0.0f, count - 1) * kButtonGap) * scale;
  auto x = panel_.right() - kPadding * scale - rowWidth;
  const auto y = panel_.bottom() - (kPadding + kButtonHeight) * scale;
  buttons_.clear();
  for (const auto &button : current_->buttons) {
    buttons_.push_back({
        .box = SkRect::MakeXYWH(x, y, kButtonWidth * scale, kButtonHeight * scale),
        .label = overlayParagraph({{.text = button.text, .size = kTextSize * scale}}, kOneLine),
    });
    x += (kButtonWidth + kButtonGap) * scale;
  }
}

void AlertDialog::focus(std::size_t index) {
  focused_ = index;
  scene_.invalidate();
}

void AlertDialog::close(std::optional<int> buttonKey) {
  auto onClose = std::move(current_->onClose);
  current_.reset();
  text_.reset();
  buttons_.clear();
  if (onClose) onClose(buttonKey);
  if (!current_) present();
  if (!current_ && std::exchange(isLayer_, false)) scene_.removeLayer(*this);
}

void AlertDialog::draw(SkCanvas &canvas) {
  if (!current_) return;
  const auto scale = static_cast<float>(scene_.size().height) / kLogicalHeight;
  canvas.drawColor(kScrim);
  canvas.drawRRect(SkRRect::MakeRectXY(panel_, kPanelRadius * scale, kPanelRadius * scale), fill(kPanel));
  text_->paint(&canvas, panel_.left() + kPadding * scale, panel_.top() + kPadding * scale);
  for (std::size_t index = 0; index < buttons_.size(); ++index) {
    const auto &button = buttons_[index];
    const auto shape = SkRRect::MakeRectXY(button.box, kButtonRadius * scale, kButtonRadius * scale);
    canvas.drawRRect(shape, fill(index == focused_ ? kFocusedButton : kButton));
    SkAutoCanvasRestore restore(&canvas, true);
    canvas.clipRRect(shape, true);
    button.label->paint(
        &canvas,
        button.box.centerX() - button.label->getLongestLine() / 2,
        button.box.centerY() - button.label->getHeight() / 2);
  }
}

} // namespace rnwebos
