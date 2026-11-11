#include "TextEngine.h"

#include "../logging/Log.h"
#include "FallbackFontManager.h"

#include "include/core/SkFontMgr.h"
#include "include/core/SkTypeface.h"
#include "include/ports/SkFontMgr_directory.h"
#include "modules/skparagraph/include/TypefaceFontProvider.h"
#include "modules/skunicode/include/SkUnicode_icu.h"

#include <cstdlib>
#include <stdexcept>

namespace rnwebos {

namespace {

constexpr const char *kSystemFontDirectory = "/usr/share/fonts";

struct Engine {
  TextEngine text;
  sk_sp<SkFontMgr> system;
  sk_sp<skia::textlayout::TypefaceFontProvider> app;
  std::vector<std::string> appFamilies;
};

// The family a paragraph falls back to when its own is missing. Which of
// these a TV has is not documented: the first one found wins.
const std::vector<SkString> &defaultFamilies() {
  static const std::vector<SkString> families{
      SkString("LG Smart UI"), SkString("Museo Sans"), SkString("Noto Sans"), SkString("DejaVu Sans")};
  return families;
}

Engine &engine() {
  static Engine *instance = [] {
    auto *created = new Engine();
    const char *directory = std::getenv("RN_WEBOS_FONT_DIR");
    created->system = withCharacterFallback(
        SkFontMgr_New_Custom_Directory(directory != nullptr ? directory : kSystemFontDirectory));
    created->app = sk_make_sp<skia::textlayout::TypefaceFontProvider>();
    created->text.fonts = sk_make_sp<skia::textlayout::FontCollection>();
    created->text.fonts->setAssetFontManager(created->app);
    created->text.fonts->setDefaultFontManager(created->system, defaultFamilies());
    created->text.fonts->enableFontFallback();
    created->text.unicode = SkUnicodes::ICU::Make();
    if (!created->text.unicode) throw std::runtime_error("ICU has no break rules: is the ICU data file in place?");
    return created;
  }();
  return *instance;
}

} // namespace

TextEngine &textEngine() {
  return engine().text;
}

int registerAppFont(const std::string &family, const std::vector<std::filesystem::path> &files) {
  auto &shared = engine();
  std::lock_guard lock(shared.text.mutex);
  int registered = 0;
  for (const auto &file : files) {
    auto typeface = shared.system->makeFromFile(file.c_str(), 0);
    if (!typeface) {
      log(LogLevel::Warning, "Not a font Skia can read: " + file.string());
      continue;
    }
    shared.app->registerTypeface(std::move(typeface), SkString(family.c_str()));
    ++registered;
  }
  if (registered > 0) {
    shared.appFamilies.push_back(family);
    shared.text.fonts->clearCaches();
  }
  return registered;
}

std::vector<std::string> fontFamilyNames() {
  auto &shared = engine();
  std::lock_guard lock(shared.text.mutex);
  std::vector<std::string> names;
  const int count = shared.system->countFamilies();
  for (int index = 0; index < count; ++index) {
    SkString name;
    shared.system->getFamilyName(index, &name);
    names.emplace_back(name.c_str());
  }
  names.insert(names.end(), shared.appFamilies.begin(), shared.appFamilies.end());
  return names;
}

} // namespace rnwebos
