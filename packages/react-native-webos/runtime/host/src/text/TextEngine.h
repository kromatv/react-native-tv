#pragma once

#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skunicode/include/SkUnicode.h"

#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace rnwebos {

/// What every paragraph is shaped and laid out with: the TV's fonts, the fonts
/// the app registered, and ICU's break rules. Building and laying out a
/// paragraph takes `mutex`: Fabric measures on its own thread while the main
/// thread lays out what it draws, and the collection's caches are not
/// thread-safe (docs/reference/skia-rendering.md, section 6).
struct TextEngine {
  std::mutex mutex;
  sk_sp<skia::textlayout::FontCollection> fonts;
  sk_sp<SkUnicode> unicode;
};

/// What `textEngine` throws when ICU's data file is not in place.
class TextEngineError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// The process's text engine, created at its first use. Any thread.
TextEngine &textEngine();

/// Makes the font files of an app (expo-font's `loadAsync`) available as
/// `family`. Returns how many of `files` were fonts. Any thread.
int registerAppFont(const std::string &family, const std::vector<std::filesystem::path> &files);

/// The families a `fontFamily` can name, the TV's first, then the app's.
std::vector<std::string> fontFamilyNames();

} // namespace rnwebos
