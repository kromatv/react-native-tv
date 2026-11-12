#include "../ExpoTestRuntime.test.h"

#include "network/Digest.h"
#include "text/AppFonts.h"

#include <Evas.h>
#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

void putU16(std::string &bytes, std::size_t at, std::uint32_t value) {
  bytes[at] = static_cast<char>((value >> 8) & 0xff);
  bytes[at + 1] = static_cast<char>(value & 0xff);
}

void putU32(std::string &bytes, std::size_t at, std::uint32_t value) {
  putU16(bytes, at, value >> 16);
  putU16(bytes, at + 2, value & 0xffff);
}

std::string staticFont(std::uint32_t weight) {
  std::string os2(78, '\0');
  putU16(os2, 4, weight);
  std::string head(54, '\0');
  putU16(head, 18, 1000);
  std::string hhea(36, '\0');
  putU16(hhea, 34, 1);
  std::string hmtx(4, '\0');
  putU16(hmtx, 0, 500);
  const std::vector<std::pair<std::string, std::string>> tables{{"OS/2", os2}, {"head", head}, {"hhea", hhea}, {"hmtx", hmtx}};
  std::string bytes(12 + 16 * tables.size(), '\0');
  putU32(bytes, 0, 0x00010000);
  putU16(bytes, 4, static_cast<std::uint32_t>(tables.size()));
  for (std::size_t index = 0; index < tables.size(); ++index) {
    const auto record = 12 + 16 * index;
    bytes.replace(record, 4, tables[index].first);
    putU32(bytes, record + 8, static_cast<std::uint32_t>(bytes.size()));
    putU32(bytes, record + 12, static_cast<std::uint32_t>(tables[index].second.size()));
    bytes += tables[index].second;
  }
  return bytes;
}

fs::path writeFont(const std::string &name, std::uint32_t weight) {
  const auto file = testing::testAppRoot() / "data" / "fonts" / name;
  fs::create_directories(file.parent_path());
  std::ofstream(file, std::ios::binary) << staticFont(weight);
  return file;
}

bool onEvasFontPath(const fs::path &directory) {
  const auto *paths = evas_font_path_global_list();
  const Eina_List *node = nullptr;
  void *data = nullptr;
  EINA_LIST_FOREACH(paths, node, data) {
    if (directory == static_cast<const char *>(data)) return true;
  }
  return false;
}

fs::path loaderDirectory(const std::string &family, const fs::path &font) {
  std::ifstream in(font, std::ios::binary);
  const std::string bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
  return testing::testAppRoot() / "cache" / "ExpoFontLoader" / md5Hex(family + "\n" + md5Hex(bytes));
}

} // namespace

TEST_CASE("loadAsync puts a font in a directory of its own on Evas's path and registers its family") {
  testing::ExpoTestRuntime expo;
  const auto font = writeFont("Brand Regular.ttf", 400);
  expo.eval("globalThis.L = expo.modules.ExpoFontLoader");
  CHECK(expo.json("[L.getLoadedFonts(), L.isLoaded('ExpoBrand')]") == "[[],false]");

  CHECK(expo.settle("L.loadAsync('ExpoBrand', 'file://" + font.parent_path().string() + "/Brand%20Regular.ttf')") == "undefined");
  CHECK(expo.json("[L.getLoadedFonts(), L.isLoaded('ExpoBrand'), L.isLoaded('Other')]") == R"([["ExpoBrand"],true,false])");
  const auto directory = loaderDirectory("ExpoBrand", font);
  CHECK(fs::is_regular_file(directory / "Brand Regular.ttf"));
  CHECK(onEvasFontPath(directory));
  CHECK(trackedAppFont("ExpoBrand", 400, false, 0.1).has_value());
}

TEST_CASE("loadFontFamilyAsync loads every face of the family, and refuses a face with no file") {
  testing::ExpoTestRuntime expo;
  const auto regular = writeFont("Family-Regular.ttf", 400);
  const auto bold = writeFont("Family-Bold.ttf", 700);
  expo.eval("globalThis.L = expo.modules.ExpoFontLoader");
  CHECK(expo.settle("L.loadFontFamilyAsync('ExpoFamily', [{ localUri: 'file://" + regular.string() +
                    "' }, { localUri: '" + bold.string() + "', weight: 700 }])") == "undefined");
  CHECK(expo.json("L.isLoaded('ExpoFamily')") == "true");
  CHECK(fs::is_regular_file(loaderDirectory("ExpoFamily", bold) / "Family-Bold.ttf"));
  CHECK_THROWS_WITH(expo.settle("L.loadFontFamilyAsync('ExpoBroken', [{ weight: 400 }])"), "ERR_FONT_SOURCE: A font face has no localUri");
  CHECK(expo.json("L.isLoaded('ExpoBroken')") == "false");
}

TEST_CASE("loadAsync refuses a font file that is not there") {
  testing::ExpoTestRuntime expo;
  const auto missing = (testing::testAppRoot() / "data" / "fonts" / "Missing.ttf").string();
  expo.eval("globalThis.L = expo.modules.ExpoFontLoader");
  CHECK_THROWS_WITH(
      expo.settle("L.loadAsync('ExpoMissing', 'file://" + missing + "')"),
      ("ERR_FONT_SOURCE: No font file for ExpoMissing at file://" + missing).c_str());
  CHECK_THROWS_WITH(
      expo.settle("L.loadAsync('ExpoRemote', 'https://example.com/font.ttf')"),
      "ERR_FONT_SOURCE: No font file for ExpoRemote at https://example.com/font.ttf");
  CHECK(expo.json("L.getLoadedFonts()") == "[]");
}

} // namespace rntizen::expo
