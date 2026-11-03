#include "LoadingBanner.h"

#include "Malloced.h"

#include <format>

namespace rntizen {

namespace {

constexpr int kHeightDivisor = 18;
constexpr int kFontDivisor = 40;

int channel(std::uint32_t color, int shift) {
  return static_cast<int>((color >> shift) & 0xff);
}

std::string hexColor(std::uint32_t argb) {
  return std::format(
      "#{:02x}{:02x}{:02x}{:02x}", channel(argb, 16), channel(argb, 8), channel(argb, 0), channel(argb, 24));
}

std::string escaped(const std::string &text) {
  const Malloced<char> markup(evas_textblock_text_utf8_to_markup(nullptr, text.c_str()));
  return markup == nullptr ? std::string() : std::string(markup.get());
}

} // namespace

LoadingBanner::LoadingBanner(Evas *evas, int screenWidth, int screenHeight)
    : fontSize_(screenHeight / kFontDivisor),
      background_(evas_object_rectangle_add(evas)),
      text_(evas_object_textblock_add(evas)),
      style_(evas_textblock_style_new()) {
  const int height = screenHeight / kHeightDivisor;
  evas_object_geometry_set(background_, 0, 0, screenWidth, height);
  evas_object_geometry_set(text_, 0, 0, screenWidth, height);
  evas_object_textblock_valign_set(text_, 0.5);
  evas_object_layer_set(background_, EVAS_LAYER_MAX - 2);
  evas_object_layer_set(text_, EVAS_LAYER_MAX - 2);
  evas_object_pass_events_set(background_, EINA_TRUE);
  evas_object_pass_events_set(text_, EINA_TRUE);
  evas_object_textblock_style_set(text_, style_);
}

LoadingBanner::~LoadingBanner() {
  evas_object_del(text_);
  evas_textblock_style_free(style_);
  evas_object_del(background_);
}

void LoadingBanner::show(const std::string &message, std::uint32_t textColor, std::uint32_t backgroundColor) {
  const auto style = std::format(
      "DEFAULT='font=Sans font_size={} color={} align=center wrap=none ellipsis=1.0'", fontSize_, hexColor(textColor));
  evas_textblock_style_set(style_, style.c_str());
  evas_object_textblock_text_markup_set(text_, escaped(message).c_str());
  const int alpha = channel(backgroundColor, 24);
  evas_object_color_set(
      background_,
      channel(backgroundColor, 16) * alpha / 255,
      channel(backgroundColor, 8) * alpha / 255,
      channel(backgroundColor, 0) * alpha / 255,
      alpha);
  evas_object_show(background_);
  evas_object_raise(text_);
  evas_object_show(text_);
}

void LoadingBanner::hide() {
  evas_object_hide(text_);
  evas_object_hide(background_);
}

} // namespace rntizen
