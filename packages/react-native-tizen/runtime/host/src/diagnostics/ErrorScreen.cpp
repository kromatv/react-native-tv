#include "ErrorScreen.h"

#include <cstdlib>

namespace rntizen {

namespace {

constexpr int kMargin = 96;
constexpr std::size_t kMaxStackLines = 18;
constexpr const char *kStyle =
    "DEFAULT='font=Sans font_size=28 color=#ffffff wrap=word'"
    " title='+ font_size=48 font_weight=bold'"
    " frame='+ font=Monospace font_size=22 color=#ffd9d9'"
    " hint='+ font_size=24 color=#ffd9d9'";

std::string escaped(const std::string &text) {
  char *markup = evas_textblock_text_utf8_to_markup(nullptr, text.c_str());
  if (markup == nullptr) return {};
  std::string result(markup);
  std::free(markup);
  return result;
}

std::string markupFor(const ErrorReport &report) {
  std::string markup = "<title>" + escaped(report.title) + "</title><br/><br/>" + escaped(report.message);
  if (!report.stack.empty()) markup += "<br/><br/>";
  for (std::size_t index = 0; index < report.stack.size() && index < kMaxStackLines; ++index) {
    markup += "<frame>" + escaped(report.stack[index]) + "</frame><br/>";
  }
  return markup + "<br/><hint>The full report is in rn-tizen logs. Press Back to close the app.</hint>";
}

} // namespace

ErrorScreen::ErrorScreen(Evas *evas) : evas_(evas) {}

ErrorScreen::~ErrorScreen() {
  if (text_ != nullptr) evas_object_del(text_);
  if (style_ != nullptr) evas_textblock_style_free(style_);
  if (background_ != nullptr) evas_object_del(background_);
}

void ErrorScreen::show(const ErrorReport &report, int screenWidth, int screenHeight) {
  if (background_ == nullptr) {
    background_ = evas_object_rectangle_add(evas_);
    evas_object_color_set(background_, 0x9b, 0x1c, 0x1c, 255);
    evas_object_layer_set(background_, EVAS_LAYER_MAX - 1);
    style_ = evas_textblock_style_new();
    evas_textblock_style_set(style_, kStyle);
    text_ = evas_object_textblock_add(evas_);
    evas_object_textblock_style_set(text_, style_);
    evas_object_layer_set(text_, EVAS_LAYER_MAX - 1);
  }
  evas_object_geometry_set(background_, 0, 0, screenWidth, screenHeight);
  evas_object_geometry_set(text_, kMargin, kMargin, screenWidth - 2 * kMargin, screenHeight - 2 * kMargin);
  evas_object_textblock_text_markup_set(text_, markupFor(report).c_str());
  evas_object_show(background_);
  evas_object_raise(text_);
  evas_object_show(text_);
}

bool ErrorScreen::isShown() const {
  return background_ != nullptr && evas_object_visible_get(background_) == EINA_TRUE;
}

} // namespace rntizen
