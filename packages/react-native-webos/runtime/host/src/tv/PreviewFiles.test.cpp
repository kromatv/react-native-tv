#include "PreviewFiles.h"

#include "AppDataDirectory.test.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace rntizen {

namespace {

namespace fs = std::filesystem;
using testing::AppDataDirectory;

} // namespace

TEST_CASE("writePreviewFile keeps the last preview Samsung took in the app's data directory") {
  const AppDataDirectory data;

  writePreviewFile(R"({"sections":[{"tiles":[1,2,3]}]})");
  writePreviewFile(R"({"sections":[]})");

  CHECK(data.read("preview.json") == R"({"sections":[]})");
}

TEST_CASE("writePreviewSource keeps the source, and forgets it with when it was last fetched") {
  const AppDataDirectory data;

  writePreviewSource(R"({"url":"https://example.tv/preview.json"})");
  CHECK(data.read("preview-source.json") == R"({"url":"https://example.tv/preview.json"})");
  std::ofstream(data.path() / "preview-fetched-at") << "1762214400";
  writePreviewSource(std::nullopt);

  CHECK_FALSE(fs::exists(data.path() / "preview-source.json"));
  CHECK_FALSE(fs::exists(data.path() / "preview-fetched-at"));
}

TEST_CASE("the preview files are skipped when the app has no data directory") {
  const AppDataDirectory data;
  data.makeUnavailable();

  CHECK_NOTHROW(writePreviewFile("{}"));
  CHECK_NOTHROW(writePreviewSource("{}"));
  CHECK_NOTHROW(writePreviewSource(std::nullopt));
  CHECK_FALSE(fs::exists(data.path()));
}

TEST_CASE("writePreviewFile gives up on a preview it cannot write") {
  const AppDataDirectory data;
  fs::create_directories(data.path() / "preview.json");

  CHECK_NOTHROW(writePreviewFile("{}"));

  CHECK(fs::is_directory(data.path() / "preview.json"));
}

} // namespace rntizen
