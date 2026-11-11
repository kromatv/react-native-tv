#include "AlertDialog.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace rntizen {

namespace {

constexpr int kPanelWidth = 1040;
constexpr int kPadding = 56;
constexpr int kButtonWidth = 260;
constexpr int kButtonHeight = 76;
constexpr int kButtonGap = 24;
constexpr int kTextToButtons = 48;
constexpr int kLayer = EVAS_LAYER_MAX - 2;
constexpr const char *kStyle =
    "DEFAULT='font=Sans font_size=30 color=#d7def0 wrap=word'"
    " title='+ font_size=42 font_weight=bold color=#ffffff'"
    " button='+ font_size=30 color=#ffffff align=center wrap=none'";

std::string escaped(const std::string &text) {
  char *markup = evas_textblock_text_utf8_to_markup(nullptr, text.c_str());
  if (markup == nullptr) return {};
  std::string result(markup);
  std::free(markup);
  return result;
}

Evas_Object *rectangle(Evas *evas, int r, int g, int b, int a) {
  auto *object = evas_object_rectangle_add(evas);
  evas_object_color_set(object, r * a / 255, g * a / 255, b * a / 255, a);
  evas_object_layer_set(object, kLayer);
  return object;
}

Evas_Object *textblock(Evas *evas, Evas_Textblock_Style *style, const std::string &markup) {
  auto *object = evas_object_textblock_add(evas);
  evas_object_textblock_style_set(object, style);
  evas_object_textblock_text_markup_set(object, markup.c_str());
  evas_object_layer_set(object, kLayer);
  return object;
}

int formattedHeight(Evas_Object *text, int width) {
  evas_object_resize(text, width, 10000);
  Evas_Coord formattedWidth = 0;
  Evas_Coord height = 0;
  evas_object_textblock_size_formatted_get(text, &formattedWidth, &height);
  return height;
}

} // namespace

AlertDialog::AlertDialog(Evas *evas) : evas_(evas), style_(evas_textblock_style_new()) {
  evas_textblock_style_set(style_, kStyle);
}

AlertDialog::~AlertDialog() {
  clear();
  evas_textblock_style_free(style_);
}

void AlertDialog::show(AlertRequest request, int screenWidth, int screenHeight) {
  screenWidth_ = screenWidth;
  screenHeight_ = screenHeight;
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
  scrim_ = rectangle(evas_, 0, 0, 0, 170);
  panel_ = rectangle(evas_, 0x1f, 0x24, 0x33, 255);
  std::string markup = "<title>" + escaped(current_->title) + "</title>";
  if (!current_->message.empty()) markup += "<br/><br/>" + escaped(current_->message);
  text_ = textblock(evas_, style_, markup);
  for (const auto &button : current_->buttons) {
    buttonBackgrounds_.push_back(rectangle(evas_, 0x2d, 0x34, 0x47, 255));
    buttonLabels_.push_back(textblock(evas_, style_, "<button>" + escaped(button.text) + "</button>"));
  }
  layout();
  focus(current_->buttons.empty() ? 0 : current_->buttons.size() - 1);
}

void AlertDialog::layout() {
  const int textWidth = kPanelWidth - 2 * kPadding;
  const int textHeight = formattedHeight(text_, textWidth);
  const int buttonsHeight = buttonBackgrounds_.empty() ? 0 : kTextToButtons + kButtonHeight;
  const int panelHeight = std::min(screenHeight_, 2 * kPadding + textHeight + buttonsHeight);
  const int panelX = (screenWidth_ - kPanelWidth) / 2;
  const int panelY = (screenHeight_ - panelHeight) / 2;
  evas_object_geometry_set(scrim_, 0, 0, screenWidth_, screenHeight_);
  evas_object_geometry_set(panel_, panelX, panelY, kPanelWidth, panelHeight);
  evas_object_geometry_set(text_, panelX + kPadding, panelY + kPadding, textWidth, textHeight);
  const auto count = static_cast<int>(buttonBackgrounds_.size());
  const int rowWidth = count * kButtonWidth + std::max(0, count - 1) * kButtonGap;
  int x = panelX + kPanelWidth - kPadding - rowWidth;
  const int y = panelY + panelHeight - kPadding - kButtonHeight;
  for (int index = 0; index < count; ++index) {
    evas_object_geometry_set(buttonBackgrounds_[index], x, y, kButtonWidth, kButtonHeight);
    const int labelHeight = formattedHeight(buttonLabels_[index], kButtonWidth);
    evas_object_geometry_set(
        buttonLabels_[index], x, y + (kButtonHeight - labelHeight) / 2, kButtonWidth, labelHeight);
    x += kButtonWidth + kButtonGap;
  }
  for (auto *object : {scrim_, panel_, text_}) evas_object_show(object);
  for (std::size_t index = 0; index < buttonBackgrounds_.size(); ++index) {
    evas_object_show(buttonBackgrounds_[index]);
    evas_object_show(buttonLabels_[index]);
  }
}

void AlertDialog::focus(std::size_t index) {
  focused_ = index;
  for (std::size_t button = 0; button < buttonBackgrounds_.size(); ++button) {
    if (button == index) {
      evas_object_color_set(buttonBackgrounds_[button], 0x3d, 0x5a, 0xfe, 255);
    } else {
      evas_object_color_set(buttonBackgrounds_[button], 0x2d, 0x34, 0x47, 255);
    }
  }
}

void AlertDialog::close(std::optional<int> buttonKey) {
  auto onClose = std::move(current_->onClose);
  clear();
  current_.reset();
  if (onClose) onClose(buttonKey);
  present();
}

void AlertDialog::clear() {
  for (auto *object : buttonLabels_) evas_object_del(object);
  for (auto *object : buttonBackgrounds_) evas_object_del(object);
  buttonLabels_.clear();
  buttonBackgrounds_.clear();
  for (auto **object : {&text_, &panel_, &scrim_}) {
    if (*object != nullptr) evas_object_del(*object);
    *object = nullptr;
  }
}

} // namespace rntizen
