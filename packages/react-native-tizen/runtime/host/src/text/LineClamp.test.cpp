#include "LineClamp.h"

#include <doctest/doctest.h>

#include <Ecore_Evas.h>

#include <string>

namespace rntizen {

namespace {

constexpr const char *kText = "one two three four five six seven eight nine ten eleven twelve";

class Paragraph {
 public:
  Paragraph() {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    canvas_ = ecore_evas_buffer_new(100, 400);
    textblock_ = evas_object_textblock_add(ecore_evas_get(canvas_));
    style_ = evas_textblock_style_new();
    evas_textblock_style_set(style_, "DEFAULT='font=Sans font_size=20 color=#000000ff wrap=word'");
    evas_object_textblock_style_set(textblock_, style_);
    evas_object_textblock_text_markup_set(textblock_, kText);
    evas_object_resize(textblock_, 100, 400);
  }
  ~Paragraph() {
    evas_object_del(textblock_);
    evas_textblock_style_free(style_);
    ecore_evas_free(canvas_);
  }
  Paragraph(const Paragraph &) = delete;
  Paragraph &operator=(const Paragraph &) = delete;

  Evas_Object *textblock() const {
    return textblock_;
  }
  std::string text() const {
    const char *markup = evas_object_textblock_text_markup_get(textblock_);
    return markup == nullptr ? std::string() : std::string(markup);
  }

 private:
  Ecore_Evas *canvas_;
  Evas_Object *textblock_;
  Evas_Textblock_Style *style_;
};

} // namespace

TEST_CASE("clampLines cuts the text after its lines and ends it with an ellipsis") {
  const Paragraph paragraph;

  clampLines(paragraph.textblock(), 2, true);

  const auto text = paragraph.text();
  CHECK(text.size() < std::string(kText).size());
  CHECK(text.ends_with("&hellip;"));
  CHECK(text.starts_with("one"));
}

TEST_CASE("clampLines without an ellipsis only cuts") {
  const Paragraph paragraph;

  clampLines(paragraph.textblock(), 1, false);

  const auto text = paragraph.text();
  CHECK(text.size() < std::string(kText).size());
  CHECK_FALSE(text.ends_with("&hellip;"));
}

TEST_CASE("clampLines leaves a paragraph that already fits") {
  const Paragraph paragraph;

  clampLines(paragraph.textblock(), 100, true);

  CHECK(paragraph.text() == kText);
}

} // namespace rntizen
