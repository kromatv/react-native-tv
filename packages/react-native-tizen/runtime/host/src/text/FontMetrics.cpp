#include "FontMetrics.h"

#include <Ecore_Evas.h>
#include <Evas.h>

#include <map>
#include <tuple>

namespace rntizen {

namespace {

using Key = std::tuple<std::string, std::string, bool, int>;

Evas_Object *probe() {
  static Evas_Object *const text = evas_object_text_add(ecore_evas_get(ecore_evas_buffer_new(1, 1)));
  return text;
}

} // namespace

FontMetrics fontMetrics(const std::string &family, const std::string &weight, bool italic, int pixelSize) {
  static std::map<Key, FontMetrics> cache;
  Key key{family, weight, italic, pixelSize};
  if (auto found = cache.find(key); found != cache.end()) return found->second;
  auto *text = probe();
  const auto font = family + ":weight=" + weight + (italic ? ":slant=italic" : "");
  evas_object_text_font_set(text, font.c_str(), pixelSize);
  evas_object_text_text_set(text, "x");
  const FontMetrics metrics{.ascent = evas_object_text_ascent_get(text), .descent = evas_object_text_descent_get(text)};
  return cache.try_emplace(std::move(key), metrics).first->second;
}

} // namespace rntizen
