#include "SubtitleOverlay.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace rntizen::expo {

namespace {

constexpr double kFontPoints = 40;

std::string markupFor(const std::string &text) {
  char *markup = evas_textblock_text_utf8_to_markup(nullptr, text.c_str());
  std::string result = markup != nullptr ? markup : "";
  std::free(markup);
  return result;
}

} // namespace

SubtitleOverlay::SubtitleOverlay(Evas *evas)
    : style_(evas_textblock_style_new()), text_(evas_object_textblock_add(evas)) {
  evas_object_textblock_style_set(text_, style_);
  evas_object_pass_events_set(text_, EINA_TRUE);
  evas_object_hide(text_);
}

SubtitleOverlay::~SubtitleOverlay() {
  if (timer_ != nullptr) ecore_timer_del(timer_);
  evas_object_del(text_);
  evas_textblock_style_free(style_);
}

Evas_Object *SubtitleOverlay::object() const {
  return text_;
}

void SubtitleOverlay::show(const std::string &text, unsigned long milliseconds) {
  if (timer_ != nullptr) {
    ecore_timer_del(timer_);
    timer_ = nullptr;
  }
  if (text.empty()) {
    clear();
    return;
  }
  evas_object_textblock_text_markup_set(text_, markupFor(text).c_str());
  shown_ = true;
  layout();
  evas_object_show(text_);
  if (milliseconds > 0) timer_ = ecore_timer_add(milliseconds / 1000.0, &SubtitleOverlay::expire, this);
}

void SubtitleOverlay::clear() {
  shown_ = false;
  evas_object_hide(text_);
}

void SubtitleOverlay::place(const PixelRect &box, double scale) {
  box_ = box;
  const int fontSize = std::max(1, static_cast<int>(std::lround(kFontPoints * scale)));
  if (fontSize != fontSize_) {
    fontSize_ = fontSize;
    const auto style = "DEFAULT='font=Sans font_size=" + std::to_string(fontSize) +
                       " color=#ffffff align=center wrap=word style=outline outline_color=#000000'";
    evas_textblock_style_set(style_, style.c_str());
  }
  layout();
}

void SubtitleOverlay::layout() {
  if (!shown_ || box_.width <= 0 || box_.height <= 0) return;
  const Evas_Coord margin = box_.width / 20;
  const Evas_Coord width = std::max(1, box_.width - 2 * margin);
  evas_object_resize(text_, width, box_.height);
  Evas_Coord textWidth = 0;
  Evas_Coord textHeight = 0;
  evas_object_textblock_size_formatted_get(text_, &textWidth, &textHeight);
  const Evas_Coord bottom = box_.y + box_.height - box_.height / 16;
  evas_object_geometry_set(text_, box_.x + margin, std::max(box_.y, bottom - textHeight), width, textHeight);
}

Eina_Bool SubtitleOverlay::expire(void *data) {
  auto *overlay = static_cast<SubtitleOverlay *>(data);
  overlay->timer_ = nullptr;
  overlay->clear();
  return ECORE_CALLBACK_CANCEL;
}

} // namespace rntizen::expo
