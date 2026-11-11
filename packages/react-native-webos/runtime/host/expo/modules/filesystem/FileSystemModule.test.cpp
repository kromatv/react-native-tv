#include "FileSystemTest.test.h"

#include "../HttpTestServer.test.h"

#include <doctest/doctest.h>

#include <sys/statvfs.h>

#include <filesystem>
#include <fstream>
#include <iterator>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

std::string contents(const fs::path &file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::string served(const std::string &request) {
  if (request.starts_with("GET /missing")) return testing::httpResponse(404, "Not Found", "no such file");
  return testing::httpResponse(200, "OK", "payload of " + request.substr(4, request.find(' ', 4) - 4));
}

} // namespace

TEST_CASE("FileSystem's constants are the app's data, cache and resource directories") {
  testing::ExpoTestRuntime expo;
  const auto root = testing::testAppRoot().string();
  CHECK(expo.json("expo.modules.FileSystem.documentDirectory") == "\"file://" + root + "/data/\"");
  CHECK(expo.json("expo.modules.FileSystem.cacheDirectory") == "\"file://" + root + "/cache/\"");
  CHECK(expo.json("expo.modules.FileSystem.bundleDirectory") == "\"" + root + "/res/\"");
}

TEST_CASE("FileSystem reports the data volume's size and free space") {
  testing::ExpoTestRuntime expo;
  struct statvfs stats {};
  REQUIRE(statvfs((testing::testAppRoot() / "data").c_str(), &stats) == 0);
  const auto total = static_cast<double>(stats.f_blocks) * static_cast<double>(stats.f_frsize);
  CHECK(expo.eval("expo.modules.FileSystem.totalDiskSpace").asNumber() == total);
  const auto available = expo.eval("expo.modules.FileSystem.availableDiskSpace").asNumber();
  CHECK(available > 0);
  CHECK(available <= total);
}

TEST_CASE("FileSystem.info says whether a URI names a file, a directory or nothing") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "module-info");
  std::ofstream(scope.directory() / "file.txt") << "x";
  CHECK(expo.json("[FS.info(root + 'file.txt').exists, FS.info(root + 'file.txt').isDirectory]") == "[true,false]");
  CHECK(expo.json("[FS.info(root).exists, FS.info(root).isDirectory]") == "[true,true]");
  CHECK(expo.json("[FS.info(root + 'none').exists, FS.info(root + 'none').isDirectory]") == "[false,null]");
  CHECK(expo.json("fails(() => FS.info('content://media/1'))") == R"("ERR_INVALID_URI: 'content://media/1' is not a local file URI")");
  CHECK(expo.json("FS.cancelDownloadAsync('any') === undefined") == "true");
}

TEST_CASE("downloadFileAsync writes the body into a Directory under the URL's name, or onto a File") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "module-download");
  testing::HttpTestServer server(served);
  const auto &root = scope.directory();
  expo.eval("globalThis.base = '" + server.url("/") + "'; globalThis.here = new FS.FileSystemDirectory(root)");

  CHECK(expo.settle("FS.downloadFileAsync(base + 'files/report.txt?version=2#top', here, null, null)") ==
        "\"file://" + root.string() + "/report.txt\"");
  CHECK(contents(root / "report.txt") == "payload of /files/report.txt?version=2");
  CHECK_THROWS_WITH(
      expo.settle("FS.downloadFileAsync(base + 'files/report.txt', here, null, null)"),
      ("ERR_DESTINATION_EXISTS: Destination '" + (root / "report.txt").string() + "' already exists").c_str());
  CHECK(expo.settle("FS.downloadFileAsync(base + 'again/report.txt', here, { idempotent: true }, 'id')") ==
        "\"file://" + root.string() + "/report.txt\"");
  CHECK(contents(root / "report.txt") == "payload of /again/report.txt");

  CHECK(expo.settle("FS.downloadFileAsync(base, here, null, null)") == "\"file://" + root.string() + "/download\"");
  CHECK(expo.settle("FS.downloadFileAsync(base + 'x.bin', new FS.FileSystemFile(root + 'chosen.bin'), null, null)") ==
        "\"file://" + root.string() + "/chosen.bin\"");
  CHECK(contents(root / "chosen.bin") == "payload of /x.bin");

  const auto failure = std::string("missing.txt answered 404");
  try {
    expo.settle("FS.downloadFileAsync(base + 'missing.txt', here, null, null)");
    FAIL("a 404 resolved");
  } catch (const std::runtime_error &error) {
    CHECK(std::string(error.what()).find(failure) != std::string::npos);
  }
  CHECK_FALSE(fs::exists(root / "missing.txt"));
}

TEST_CASE("downloadFileAsync names the file by the URL's decoded name and returns a URI that names it") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "module-download-names");
  testing::HttpTestServer server(served);
  expo.eval("globalThis.base = '" + server.url("/") + "'");
  const auto uri = expo.settle("FS.downloadFileAsync(base + 'my%20file%23%31.txt', new FS.FileSystemDirectory(root), null, null)");
  CHECK(uri == "\"file://" + scope.directory().string() + "/my%20file%231.txt\"");
  CHECK(contents(scope.directory() / "my file#1.txt") == "payload of /my%20file%23%31.txt");
  CHECK(expo.json("new FS.FileSystemFile(" + uri + ").textSync()") == R"("payload of /my%20file%23%31.txt")");
}

} // namespace rntizen::expo
