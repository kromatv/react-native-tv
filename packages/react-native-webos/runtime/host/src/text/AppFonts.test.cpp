#include "AppFonts.h"

#include "AppFont.test.h"

#include <doctest/doctest.h>

#include <filesystem>

namespace rntizen {

namespace {

namespace fs = std::filesystem;

using testing::appFont;
using testing::writeFile;

} // namespace

TEST_CASE("trackedAppFont writes the closest file with its advances moved by the spacing, once") {
  const auto root = fs::temp_directory_path() / "rn-tizen-app-fonts-test";
  fs::remove_all(root);
  fs::create_directories(root / "Brand");
  const auto regular = writeFile(root / "Brand" / "Regular.ttf", appFont(400, 1000));
  const auto bold = writeFile(root / "Brand" / "Bold.ttf", appFont(700, 1000));
  const auto broken = writeFile(root / "Brand" / "Broken.ttf", "not a font");
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
  registerAppFontFiles("Small", {writeFile(root / "Small" / "Regular.ttf", appFont(400, 1000))});

  CHECK_FALSE(trackedAppFont("Unknown", 400, false, 0.01).has_value());
  CHECK_FALSE(trackedAppFont("Small", 400, false, 0.0001).has_value());
  fs::remove_all(root);
}

} // namespace rntizen
