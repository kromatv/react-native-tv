#include "ImageCache.h"

#include "../../ExpoTestRuntime.test.h"
#include "../HttpTestServer.test.h"

#include "network/Digest.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

std::string contents(const fs::path &file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

fs::path cached(const std::string &key) {
  return testing::testAppRoot() / "cache" / "ExpoImage" / md5Hex(key);
}

std::string image(const std::string &request) {
  if (request.starts_with("GET /missing")) return testing::httpResponse(404, "Not Found", "");
  return testing::httpResponse(200, "OK", "pixels of " + request.substr(4, request.find(' ', 4) - 4));
}

} // namespace

TEST_CASE("imageFileFor downloads a remote image once, then serves it from the disk cache") {
  clearImageDiskCache();
  testing::HttpTestServer server(image);
  const auto url = server.url("/photo.jpg");

  const auto first = imageFileFor(url, "");
  CHECK(first.cacheType == "none");
  CHECK(first.path == cached(url).string());
  CHECK(contents(first.path) == "pixels of /photo.jpg");
  const auto second = imageFileFor(url, "");
  CHECK(second.cacheType == "disk");
  CHECK(second.path == first.path);
  CHECK(server.requests().size() == 1);

  const auto keyed = imageFileFor(server.url("/other.jpg"), "my-key");
  CHECK(keyed.path == cached("my-key").string());
  CHECK(cachedImagePath("my-key") == cached("my-key").string());
  CHECK_FALSE(cachedImagePath("never-loaded").has_value());

  std::ofstream(cached(url), std::ios::trunc).flush();
  CHECK(imageFileFor(url, "").cacheType == "none");
  CHECK(server.requests().size() == 3);
}

TEST_CASE("imageFileFor reads a local file where it is, and refuses what it cannot load") {
  const auto file = testing::testAppRoot() / "data" / "local image.png";
  std::ofstream(file, std::ios::binary) << "local pixels";
  const auto local = imageFileFor("file://" + (testing::testAppRoot() / "data").string() + "/local%20image.png", "");
  CHECK(local.path == file.string());
  CHECK(local.cacheType == "disk");
  CHECK_THROWS_WITH_AS(imageFileFor("file:///no/such/image.png", ""), "No image file at file:///no/such/image.png", std::runtime_error);
  CHECK_THROWS_WITH_AS(imageFileFor("ftp://host/image.png", ""), "Unsupported image source ftp://host/image.png", std::runtime_error);
  CHECK_THROWS_WITH_AS(imageFileFor("data:image/png;base64,AAAA", ""), "Unsupported image source data:image/png;base64,AAAA", std::runtime_error);
  testing::HttpTestServer server(image);
  CHECK_THROWS_AS(imageFileFor(server.url("/missing.png"), ""), std::runtime_error);
  fs::remove(file);
}

TEST_CASE("ExpoImage prefetches every URL, and answers false when one cannot be had") {
  clearImageDiskCache();
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server(image);
  expo.eval("globalThis.I = expo.modules.ExpoImage; globalThis.base = '" + server.url("/") + "'");

  CHECK(expo.settle("I.prefetch([base + 'a.png', base + 'b.png'], 'disk', null)") == "true");
  CHECK(fs::is_regular_file(cached(server.url("/a.png"))));
  CHECK(fs::is_regular_file(cached(server.url("/b.png"))));
  CHECK(expo.settle("I.prefetch([base + 'a.png'], null, { Authorization: 'x' })") == "true");
  CHECK(server.requests().size() == 2);
  CHECK(expo.settle("I.prefetch([base + 'c.png', base + 'missing.png', 'ftp://host/d.png'], null, null)") == "false");
  CHECK(fs::is_regular_file(cached(server.url("/c.png"))));
  CHECK(expo.settle("I.prefetch([], null, null)") == "true");
}

TEST_CASE("ExpoImage finds a cached image by its key and clears the disk cache") {
  clearImageDiskCache();
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server(image);
  const auto url = server.url("/cached.png");
  expo.eval("globalThis.I = expo.modules.ExpoImage");

  CHECK(expo.settle("I.getCachePathAsync('" + url + "')") == "null");
  CHECK(expo.settle("I.prefetch(['" + url + "'], null, null)") == "true");
  CHECK(expo.settle("I.getCachePathAsync('" + url + "')") == "\"" + cached(url).string() + "\"");
  CHECK(expo.settle("I.clearMemoryCache()") == "true");
  CHECK(expo.settle("I.clearDiskCache()") == "true");
  CHECK_FALSE(fs::exists(cached(url)));
  CHECK(expo.settle("I.clearDiskCache()") == "false");
}

} // namespace rntizen::expo
