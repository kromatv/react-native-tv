#include "BundleLoader.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace rntizen {

namespace fs = std::filesystem;

TEST_CASE("loadBundleFile reads the packaged bundle and reports it as a file URL") {
  const auto file = fs::temp_directory_path() / "rn-tizen-bundle-test.bundle";
  std::ofstream(file, std::ios::binary) << "globalThis.ready = true;";

  const auto bundle = loadBundleFile(file.string());

  CHECK(std::string(bundle.script->c_str(), bundle.script->size()) == "globalThis.ready = true;");
  CHECK(bundle.sourceURL == "file://" + file.string());
  fs::remove(file);
}

TEST_CASE("loadBundleFile names a missing bundle") {
  CHECK_THROWS_AS(loadBundleFile("/nonexistent/index.bundle"), BundleError);
}

} // namespace rntizen
