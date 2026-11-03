#include "AppFonts.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace rntizen {

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

std::string font(std::uint32_t weight, std::uint32_t unitsPerEm) {
  std::string os2(78, '\0');
  putU16(os2, 4, weight);
  std::string head(54, '\0');
  putU16(head, 18, unitsPerEm);
  std::string hhea(36, '\0');
  putU16(hhea, 34, 1);
  std::string hmtx(4, '\0');
  putU16(hmtx, 0, 500);
  const std::vector<std::pair<std::string, std::string>> tables{
      {"OS/2", os2}, {"head", head}, {"hhea", hhea}, {"hmtx", hmtx}};
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

fs::path write(const fs::path &file, const std::string &bytes) {
  std::ofstream(file, std::ios::binary) << bytes;
  return file;
}

} // namespace

TEST_CASE("trackedAppFont writes the closest file with its advances moved by the spacing, once") {
  const auto root = fs::temp_directory_path() / "rn-tizen-app-fonts-test";
  fs::remove_all(root);
  fs::create_directories(root / "Brand");
  const auto regular = write(root / "Brand" / "Regular.ttf", font(400, 1000));
  const auto bold = write(root / "Brand" / "Bold.ttf", font(700, 1000));
  const auto broken = write(root / "Brand" / "Broken.ttf", "not a font");
  registerAppFontFiles("Brand", {regular, bold, broken});

  const auto tracked = trackedAppFont("Brand", 600, false, 0.01);

  REQUIRE(tracked.has_value());
  CHECK(*tracked == root / "tracked" / "Brand-Bold-10.ttf");
  CHECK(fs::file_size(*tracked) == fs::file_size(bold));
  CHECK(trackedAppFont("Brand", 600, false, 0.01) == tracked);
  fs::remove_all(root);
}

TEST_CASE("trackedAppFont has nothing for an unknown family or a spacing under one font unit") {
  const auto root = fs::temp_directory_path() / "rn-tizen-app-fonts-small";
  fs::remove_all(root);
  fs::create_directories(root / "Small");
  registerAppFontFiles("Small", {write(root / "Small" / "Regular.ttf", font(400, 1000))});

  CHECK_FALSE(trackedAppFont("Unknown", 400, false, 0.01).has_value());
  CHECK_FALSE(trackedAppFont("Small", 400, false, 0.0001).has_value());
  fs::remove_all(root);
}

} // namespace rntizen
