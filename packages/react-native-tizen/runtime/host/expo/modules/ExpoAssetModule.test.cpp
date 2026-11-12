#include "../ExpoTestRuntime.test.h"
#include "HttpTestServer.test.h"

#include "network/Digest.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

std::string contents(const fs::path &file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

fs::path assetPath(const std::string &key, const std::string &type) {
  return testing::testAppRoot() / "cache" / ("ExponentAsset-" + key + "." + type);
}

} // namespace

TEST_CASE("downloadAsync hands back a local asset as a file URI") {
  testing::ExpoTestRuntime expo;
  expo.eval("globalThis.A = expo.modules.ExpoAsset");
  CHECK(expo.settle("A.downloadAsync('file:///opt/usr/apps/app/res/logo.png', null, 'png')") ==
        R"("file:///opt/usr/apps/app/res/logo.png")");
  CHECK(expo.settle("A.downloadAsync('/opt/usr/apps/app/res/logo.png', null, 'png')") ==
        R"("file:///opt/usr/apps/app/res/logo.png")");
}

TEST_CASE("downloadAsync caches a remote asset under the MD5 of its URL and reuses it") {
  testing::ExpoTestRuntime expo;
  int served = 0;
  testing::HttpTestServer server([&served](const std::string &) {
    ++served;
    return testing::httpResponse(200, "OK", "image " + std::to_string(served));
  });
  const auto url = server.url("/logo.png");
  const auto path = assetPath(md5Hex(url), "png");
  fs::remove(path);
  expo.eval("globalThis.A = expo.modules.ExpoAsset");

  CHECK(expo.settle("A.downloadAsync('" + url + "', null, 'png')") == "\"file://" + path.string() + "\"");
  CHECK(contents(path) == "image 1");
  CHECK(expo.settle("A.downloadAsync('" + url + "', null, 'png')") == "\"file://" + path.string() + "\"");
  CHECK(server.requests().size() == 1);
  fs::remove(path);
}

TEST_CASE("downloadAsync keys by the MD5 it is given and downloads again when the file does not match it") {
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server([](const std::string &) { return testing::httpResponse(200, "OK", "font bytes"); });
  const auto url = server.url("/font.ttf");
  const auto hash = md5Hex("font bytes");
  const auto path = assetPath(hash, "ttf");
  fs::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << "stale";
  expo.eval("globalThis.A = expo.modules.ExpoAsset");

  CHECK(expo.settle("A.downloadAsync('" + url + "', '" + hash + "', 'ttf')") == "\"file://" + path.string() + "\"");
  CHECK(contents(path) == "font bytes");
  CHECK(expo.settle("A.downloadAsync('" + url + "', '" + hash + "', 'ttf')") == "\"file://" + path.string() + "\"");
  CHECK(server.requests().size() == 1);
  fs::remove(path);

  const auto untyped = assetPath(md5Hex(url), "");
  CHECK(expo.settle("A.downloadAsync('" + url + "', null, null)") == "\"file://" + untyped.string() + "\"");
  fs::remove(untyped);
}

TEST_CASE("downloadAsync rejects what the server will not give") {
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server([](const std::string &) { return testing::httpResponse(404, "Not Found", "gone"); });
  const auto url = server.url("/gone.png");
  expo.eval("globalThis.A = expo.modules.ExpoAsset");
  try {
    expo.settle("A.downloadAsync('" + url + "', null, 'png')");
    FAIL("a 404 resolved");
  } catch (const std::runtime_error &error) {
    CHECK(std::string(error.what()).find(url + " answered 404") != std::string::npos);
  }
  CHECK_FALSE(fs::exists(assetPath(md5Hex(url), "png")));
}

} // namespace rntizen::expo
