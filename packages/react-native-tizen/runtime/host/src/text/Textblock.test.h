#pragma once

#include <Ecore_Evas.h>
#include <doctest/doctest.h>

#include <string>

namespace rntizen::testing {

class Textblock final {
 public:
  Textblock(const std::string &style, const std::string &markup, int width, int height)
      : object_(evas_object_textblock_add(canvas())), style_(evas_textblock_style_new()) {
    evas_textblock_style_set(style_, ("DEFAULT='" + style + "'").c_str());
    evas_object_textblock_style_set(object_, style_);
    evas_object_textblock_text_markup_set(object_, markup.c_str());
    evas_object_resize(object_, width, height);
  }

  ~Textblock() {
    evas_object_del(object_);
    evas_textblock_style_free(style_);
  }

  Textblock(const Textblock &) = delete;
  Textblock &operator=(const Textblock &) = delete;

  Evas_Object *object() const { return object_; }

  std::string markup() const {
    const char *markup = evas_object_textblock_text_markup_get(object_);
    return markup == nullptr ? std::string() : std::string(markup);
  }

  int lineCount() const {
    int lines = 0;
    while (evas_object_textblock_line_number_geometry_get(object_, lines, nullptr, nullptr, nullptr, nullptr)) ++lines;
    return lines;
  }

 private:
  static Evas *canvas() {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    static Evas *const shared = [] {
      auto *buffer = ecore_evas_buffer_new(1, 1);
      ecore_evas_input_event_unregister(buffer);
      return ecore_evas_get(buffer);
    }();
    return shared;
  }

  Evas_Object *object_;
  Evas_Textblock_Style *style_;
};

} // namespace rntizen::testing
