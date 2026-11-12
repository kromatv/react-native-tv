#include "../ExpoTestRuntime.test.h"

#include "text/TextEngine.h"

#include <doctest/doctest.h>
#include <unicode/putil.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

namespace rnwebos::expo {

namespace {

namespace fs = std::filesystem;

// The text engine breaks lines with ICU's rules, which are in the data file
// an app ships and not in the test binary.
const bool icuDataReady = [] {
  u_setDataDirectory(RN_WEBOS_TEST_ICU_DATA_DIR);
  return true;
}();

fs::path copyOfSkiaFont(const char *font, const std::string &name) {
  const auto file = testing::testAppRoot() / "data" / "fonts" / name;
  fs::create_directories(file.parent_path());
  fs::copy_file(fs::path(RN_WEBOS_TEST_FONT_DIR) / font, file, fs::copy_options::overwrite_existing);
  return file;
}

bool isFontFamily(const std::string &family) {
  const auto names = fontFamilyNames();
  return std::ranges::find(names, family) != names.end();
}

} // namespace

TEST_CASE("loadAsync registers a font file as its family with the text engine") {
  testing::ExpoTestRuntime expo;
  const auto font = copyOfSkiaFont("Roboto-Regular.ttf", "Brand Regular.ttf");
  expo.eval("globalThis.L = expo.modules.ExpoFontLoader");
  CHECK(expo.json("[L.getLoadedFonts(), L.isLoaded('ExpoBrand')]") == "[[],false]");
  CHECK_FALSE(isFontFamily("ExpoBrand"));

  CHECK(expo.settle("L.loadAsync('ExpoBrand', 'file://" + font.parent_path().string() + "/Brand%20Regular.ttf')") == "undefined");

  CHECK(expo.json("[L.getLoadedFonts(), L.isLoaded('ExpoBrand'), L.isLoaded('Other')]") == R"([["ExpoBrand"],true,false])");
  CHECK(isFontFamily("ExpoBrand"));
}

TEST_CASE("loadFontFamilyAsync loads every face of the family, and refuses a face with no file") {
  testing::ExpoTestRuntime expo;
  const auto regular = copyOfSkiaFont("Roboto-Regular.ttf", "Family-Regular.ttf");
  const auto variable = copyOfSkiaFont("Variable.ttf", "Family-Variable.ttf");
  expo.eval("globalThis.L = expo.modules.ExpoFontLoader");

  CHECK(expo.settle("L.loadFontFamilyAsync('ExpoFamily', [{ localUri: 'file://" + regular.string() +
                    "' }, { localUri: '" + variable.string() + "', weight: 700 }])") == "undefined");
  CHECK(expo.json("L.isLoaded('ExpoFamily')") == "true");
  CHECK(isFontFamily("ExpoFamily"));
  CHECK_THROWS_WITH(expo.settle("L.loadFontFamilyAsync('ExpoBroken', [{ weight: 400 }])"), "ERR_FONT_SOURCE: A font face has no localUri");
  CHECK(expo.json("L.isLoaded('ExpoBroken')") == "false");
}

TEST_CASE("loadAsync refuses a font file that is not there, and a file that is not a font") {
  testing::ExpoTestRuntime expo;
  const auto missing = (testing::testAppRoot() / "data" / "fonts" / "Missing.ttf").string();
  const auto text = testing::testAppRoot() / "data" / "fonts" / "NotAFont.ttf";
  fs::create_directories(text.parent_path());
  std::ofstream(text) << "not a font";
  expo.eval("globalThis.L = expo.modules.ExpoFontLoader");

  CHECK_THROWS_WITH(
      expo.settle("L.loadAsync('ExpoMissing', 'file://" + missing + "')"),
      ("ERR_FONT_SOURCE: No font file for ExpoMissing at file://" + missing).c_str());
  CHECK_THROWS_WITH(
      expo.settle("L.loadAsync('ExpoRemote', 'https://example.com/font.ttf')"),
      "ERR_FONT_SOURCE: No font file for ExpoRemote at https://example.com/font.ttf");
  CHECK_THROWS_WITH(
      expo.settle("L.loadAsync('ExpoText', 'file://" + text.string() + "')"),
      ("ERR_FONT_FILE: The file for ExpoText at file://" + text.string() + " is not a font").c_str());
  CHECK(expo.json("L.getLoadedFonts()") == "[]");
  CHECK_FALSE(isFontFamily("ExpoText"));
}

} // namespace rnwebos::expo
